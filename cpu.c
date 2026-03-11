#include "cpu.h"
#include "interrupciones.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

extern int g_modo_debug;

//Inicializa la CPU a 0 
void cpu_inicializar(CPU_t *cpu) {
    cpu->AC = 0;
    cpu->MAR = 0;
    cpu->MDR = 0;
    cpu->IR = 0;
    cpu->RB = MEM_SO;  //Lo primero que se ejecuta al iniciar el sistema es el SO, el registro base se igual a el, ya que contiene la pos de mem del proceso en ejecucion
    cpu->RX = 0;
    cpu->SP = 0;
    cpu->PSW.codigo_condicion = 0;     //El codigo de condicion del PSW se inicializa a 0
    cpu->PSW.modo = MODO_KERNEL;       //La CPU siempre debe incializarse en modo kernel
    cpu->PSW.interrupciones = INT_HABILITADAS;  //La CPU reacciona a señales externas
    cpu->PSW.pc = MEM_SO;              //Comienza a leer donde se carga el SO
    
    log_mensaje("CPU inicializada");
}

//------------------------------------------------------CICLOS DE INSTRUCCION DE LA CPU----------------------------------------------------------------------------------


void cpu_busqueda(CPU_t *cpu, palabra_t *memoria) {  //Indica lo primero que debe hacer la CPU
    // Verificar proteccion de memoria (si estamos en modo usuario)
    if (cpu->PSW.modo == MODO_USUARIO) {
        // Verificar si la direccion de la instruccion esta protegida
        if (cpu->PSW.pc > cpu->RL || cpu->PSW.pc < cpu->RB) {
            lanzar_interrupcion(INT_DIR_INVALIDA); // Arrojar excepcion codigo 6
            return;
        }

        // Verificar que no se ejecute codigo en la pila
        // La pila comienza en RX. Si PC >= RX, estamos intentando ejecutar datos de pila como instrucciones.
        if (cpu->PSW.pc >= cpu->RX) {
            // Se considera una violacion de acceso o instruccion invalida.
            // Usamos INT_DIR_INVALIDA porque estamos accediendo a una zona de memoria prohibida para ejecucion.
            log_error("Intento de ejecucion en la pila", cpu->PSW.pc);
            lanzar_interrupcion(INT_DIR_INVALIDA);
            return;
        }
    }

    // MAR obtiene PC
    cpu->MAR = cpu->PSW.pc;
    
    // MDR obtiene contenido de memoria[MAR]
    cpu->MDR = memoria[cpu->MAR];
    
    // IR obtiene MDR
    cpu->IR = cpu->MDR;
    
    // Obtiene la siguiente instruccion
    cpu->PSW.pc++;
    
    //Imprime el estado actual de los registros 
    if (g_modo_debug) {
        printf("FETCH: MAR=%d, MDR=%08d, IR=%08d, PC=%d\n", 
               cpu->MAR, cpu->MDR, cpu->IR, cpu->PSW.pc);
    }
}

Instruccion_t cpu_decodificar_instruccion(palabra_t instruccion_raw) {
    Instruccion_t inst;
    
    inst.codigo_op = instruccion_raw / 1000000;        //Al dividir entre 1000000 se obtiene el codigo de la instruccion
    inst.direccionamiento = (instruccion_raw / 100000) % 10;     //Obtiene el tipo de direccionamiento (0 o 1)
    inst.valor = instruccion_raw % 100000;             //Obtiene el dato con el que se trabaja 
    
    return inst;
}

int cpu_calcular_direccion(CPU_t *cpu, Instruccion_t inst) {     //Calcula la direccion de destino
    int direccion = 0;
    
    //Se decide como calcular basandose en el campo direccionamiento que extrajimos en la fase de decode:
    switch(inst.direccionamiento) {
        case DIR_DIRECTO:
            direccion = inst.valor;       //Toma la direccion de la instruccion
            break;
        case DIR_INMEDIATO:
            direccion = -1; // No aplica ya que la direccion es el dato directamente
            break;
        case DIR_INDEXADO:
            direccion = cpu->AC + inst.valor;   //Suma el contenido del registro AC + el valor de la instruccion.
            break;
    }
    
    return direccion;
}

palabra_t cpu_obtener_operando(CPU_t *cpu, Instruccion_t inst, palabra_t *memoria) {
    palabra_t operando = 0;
    
    switch(inst.direccionamiento) {  //Dependiento del tipo de direccionamiento actuara

        case DIR_DIRECTO:
            if (cpu->PSW.modo == MODO_USUARIO) {
                int dir_fisica = cpu->RB + inst.valor;
                if (!cpu_verificar_memoria(cpu, dir_fisica)) {  // Si el programa intenta acceder a una direccion fuera de limite
                    lanzar_interrupcion(INT_DIR_INVALIDA);
                    return 0;
                }
                operando = memoria[dir_fisica];
            } else {
                operando = memoria[inst.valor];
            }
            break;
        case DIR_INMEDIATO:
            operando = inst.valor;
            break;
        case DIR_INDEXADO:
            if (cpu->PSW.modo == MODO_USUARIO) {
                int dir_fisica = cpu->RB + cpu->AC + inst.valor;
                if (!cpu_verificar_memoria(cpu, dir_fisica)) {
                    lanzar_interrupcion(INT_DIR_INVALIDA);
                    return 0;
                }
                operando = memoria[dir_fisica];
            } else {
                operando = memoria[cpu->AC + inst.valor];
            }
            break;
    }
    
    return operando;
}

void cpu_saltar(CPU_t *cpu, int direccion_destino_relativa) {
    int dir_fisica = direccion_destino_relativa;

    // Verificar proteccion de memoria para Modo Usuario
    if (cpu->PSW.modo == MODO_USUARIO) {
        dir_fisica = cpu->RB + direccion_destino_relativa;

        if (!cpu_verificar_memoria(cpu, dir_fisica)) {
            lanzar_interrupcion(INT_DIR_INVALIDA);
            return;
        }
    }
    
    cpu->PSW.pc = dir_fisica;
}

int cpu_verificar_memoria(CPU_t *cpu, int direccion) {   //Recibe el estado de la CPU y la direccion fisica que ya calculamos
    return (direccion >= cpu->RB && direccion <= cpu->RL);  //verifica que la direccion este entre RB y RL
}

void cpu_actualizar_cc(CPU_t *cpu, palabra_t res) {
    // Detectar overflow (mas de 7 digitos de magnitud)
    if (abs(res) > 9999999) {
        cpu->PSW.codigo_condicion = CC_OVERFLOW;   //Detecta un desbordamiento 
        lanzar_interrupcion(INT_OVERFLOW);
    } else if (res == 0) {              
        cpu->PSW.codigo_condicion = CC_IGUAL;     //El res de la operacion fue 0, el cod de condicion es 0
    } else if (res < 0) {
        cpu->PSW.codigo_condicion = CC_MENOR;     //El res de la operacion es negativo, el cod de condicion 1
    } else {
        cpu->PSW.codigo_condicion = CC_MAYOR;     //El res de la operacion es positivo, el cod de condicion 2
    }
}

void cpu_ejecutar(CPU_t *cpu, Instruccion_t inst, palabra_t *memoria, ControladorDMA_t *dma) {
    palabra_t operando;
    palabra_t res;
    int direccion;
    int dir_fisica;
    
    if (g_modo_debug) {
        printf("EXECUTE: OP=%02d, DIR=%d, VAL=%05d\n", 
               inst.codigo_op, inst.direccionamiento, inst.valor);
    }
    
    switch(inst.codigo_op) {
        case 0: // sum
            operando = cpu_obtener_operando(cpu, inst, memoria); //Trae el dato (segun el direccionamiento)
            res = cpu->AC + operando;                      // Hace la suma 
            cpu_actualizar_cc(cpu, res);                   // Actualiza el codigo de condicion
            cpu->AC = res;                                 // Guarda el res en AC
            log_operacion("SUM", cpu->AC, operando, res);  // Registra la actividad en el log
            break;
            
        case 1: // res
            operando = cpu_obtener_operando(cpu, inst, memoria);
            res = cpu->AC - operando;
            cpu_actualizar_cc(cpu, res);
            cpu->AC = res;
            log_operacion("RES", cpu->AC, operando, res);
            break;
            
        case 2: // mult
            operando = cpu_obtener_operando(cpu, inst, memoria);
            res = cpu->AC * operando;
            cpu_actualizar_cc(cpu, res);
            cpu->AC = res;
            log_operacion("MULT", cpu->AC, operando, res);
            break;
            
        case 3: // divi
            operando = cpu_obtener_operando(cpu, inst, memoria);
            if (operando == 0) {
                log_operacion("DIVI", cpu->AC, operando, 0);
                lanzar_interrupcion(INT_OVERFLOW);
            } else {
                res = cpu->AC / operando;
                cpu_actualizar_cc(cpu, res);
                cpu->AC = res;
                log_operacion("DIVI", cpu->AC, operando, res);
            }
            break;
            
        case 4: // load
            operando = cpu_obtener_operando(cpu, inst, memoria);  // Copia un dato de la RAM al registro AC.
            cpu->AC = operando;
            log_operacion("LOAD", cpu->AC, operando, cpu->AC);
            break;
            
        case 5: // str copia el valor de AC a la RAM.
            direccion = cpu_calcular_direccion(cpu, inst);
            if (cpu->PSW.modo == MODO_USUARIO) {
                dir_fisica = cpu->RB + direccion;
                if (!cpu_verificar_memoria(cpu, dir_fisica)) {
                    lanzar_interrupcion(INT_DIR_INVALIDA);
                    break;
                }
                memoria[dir_fisica] = cpu->AC;
            } else {
                memoria[direccion] = cpu->AC;
            }
            log_operacion("STR", cpu->AC, direccion, memoria[direccion]);
            break;
            
        case 6: // loadrx
            cpu->AC = cpu->RX;
            log_operacion("LOADRX", cpu->AC, cpu->RX, cpu->AC);
            break;
            
        case 7: // strrx
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Si el usuario intenta cambiar la base de su pila.
                // Verificamos que la nueva direccion base (contenido de AC) 
                // caiga dentro de su particion de memoria asignada (RB a RL).
                if (!cpu_verificar_memoria(cpu, cpu->AC)) {
                    // Si intenta apuntar fuera de su memoria, lanzamos error y abortamos
                    lanzar_interrupcion(INT_DIR_INVALIDA);
                    break;
                }
            }
            cpu->RX = cpu->AC;
            log_operacion("STRRX", cpu->AC, cpu->RX, cpu->RX);
            break;
            
        case 8: // comp
            operando = cpu_obtener_operando(cpu, inst, memoria);
            res = cpu->AC - operando;
            cpu_actualizar_cc(cpu, res);
            log_operacion("COMP", cpu->AC, operando, res);
            break;
            
        case 9: // jmpe
            if (cpu->PSW.codigo_condicion == CC_IGUAL) {
                operando = cpu_obtener_operando(cpu, inst, memoria);
                
                if (!interrupcion_pendiente) {
                    // Ejecutar el salto
                    cpu_saltar(cpu, operando);
                    log_operacion("JMPE", cpu->AC, operando, cpu->PSW.pc);
                }
            }
            break;
            
        case 10: // jmpne
            if (cpu->PSW.codigo_condicion != CC_IGUAL) {
                operando = cpu_obtener_operando(cpu, inst, memoria);
                if (!interrupcion_pendiente) {
                    cpu_saltar(cpu, operando);
                    log_operacion("JMPNE", cpu->AC, operando, cpu->PSW.pc);
                }
            }
            break;
            
        case 11: // jmplt
            if (cpu->PSW.codigo_condicion == CC_MENOR) {
                operando = cpu_obtener_operando(cpu, inst, memoria);
                if (!interrupcion_pendiente) {
                    cpu_saltar(cpu, operando);
                    log_operacion("JMPLT", cpu->AC, operando, cpu->PSW.pc);
                }
            }
            break;
            
        case 12: // jmpgt
            if (cpu->PSW.codigo_condicion == CC_MAYOR) {
                operando = cpu_obtener_operando(cpu, inst, memoria);
                if (!interrupcion_pendiente) {
                    cpu_saltar(cpu, operando);
                    log_operacion("JMPGT", cpu->AC, operando, cpu->PSW.pc);
                }
            }
            break;
            
        case 13: // svc
            log_operacion("SVC", cpu->AC, 0, 0);
            lanzar_interrupcion(INT_SYSCALL);
            break;
            
        case 14: // retrn
            // Validar Underflow
            if (cpu->SP <= 0) {
                lanzar_interrupcion(INT_UNDERFLOW);
                break;
            }

            int dir_stack = cpu->RX + cpu->SP;
            
            // Verificacion de limites fisicos
            if (dir_stack < 0 || dir_stack >= TAM_MEMORIA) {
                 lanzar_interrupcion(INT_DIR_INVALIDA); 
                 break;
            }

            if (cpu->PSW.modo == MODO_USUARIO) {
                if (!cpu_verificar_memoria(cpu, dir_stack)) {
                    lanzar_interrupcion(INT_DIR_INVALIDA);
                    break;
                }
            }

            cpu->PSW.pc = memoria[dir_stack];
            cpu->SP--;
            
            log_operacion("RETRN", cpu->PSW.pc, cpu->SP, cpu->PSW.pc);
            break;
            
        case 15: // hab
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede habilitar las interrupciones
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            cpu->PSW.interrupciones = INT_HABILITADAS;
            log_mensaje("Interrupciones habilitadas");
            break;
            
        case 16: // dhab
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede desabilitar las interrupciones
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            cpu->PSW.interrupciones = INT_DESHABILITADAS;
            log_mensaje("Interrupciones deshabilitadas");
            break;
            
        case 17: // tti - establecer tiempo de reloj
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede establecer el tiempo de reloj
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            // Se maneja en el sistema principal
            log_operacion("TTI", inst.valor, 0, 0);
            break;
            
        case 18: // chmod
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede cambiar su propio modo
                lanzar_interrupcion(INT_INST_INVALIDA);
                break;
            }
            log_operacion("CHMOD", cpu->PSW.modo, 0, 0);
            break;
            
        case 19: // loadrb
            cpu->AC = cpu->RB;
            log_operacion("LOADRB", cpu->AC, cpu->RB, cpu->AC);
            break;
            
        case 20: // strrb
            if (cpu->PSW.modo == MODO_USUARIO) {
                    // Un usuario NO puede cambiar su propio registro base
                    lanzar_interrupcion(INT_INST_INVALIDA); 
                    break;
            }
            cpu->RB = cpu->AC;
            log_operacion("STRRB", cpu->AC, cpu->RB, cpu->RB);
            break;
            
        case 21: // loadrl
            cpu->AC = cpu->RL;
            log_operacion("LOADRL", cpu->AC, cpu->RL, cpu->AC);
            break;
            
        case 22: // strrl
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede cambiar su propio registro limite
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            cpu->RL = cpu->AC;
            log_operacion("STRRL", cpu->AC, cpu->RL, cpu->RL);
            break;
            
        case 23: // loadsp
            cpu->AC = cpu->SP;
            log_operacion("LOADSP", cpu->AC, cpu->SP, cpu->AC);
            break;
            
        case 24: // strsp

            // Verificar si estamos en MODO USUARIO
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Calculamos donde caeria fisicamente ese puntero
                int dir_fisica_nueva = cpu->RX + cpu->AC;
                
                // Verificamos "las direcciones": ¿Esta entre RB y RL?
                if (!cpu_verificar_memoria(cpu, dir_fisica_nueva)) {
                    // Si el usuario intenta poner el SP fuera de su memoria asignada
                    lanzar_interrupcion(INT_DIR_INVALIDA); 
                    break; // No actualizamos el SP
                }
            }
            cpu->SP = cpu->AC;
            log_operacion("STRSP", cpu->AC, cpu->SP, cpu->SP);
            break;
            
        case 25: // psh
            // Calcular la proxima posicion del SP
            int proximo_sp = cpu->SP + 1;
            dir_fisica = cpu->RX + proximo_sp;

            // Verificar si estamos en MODO USUARIO
            if (cpu->PSW.modo == MODO_USUARIO) {

                // Verificar si esa direccion fisica es valida para este proceso
                if (!cpu_verificar_memoria(cpu, dir_fisica)) {
                    // Si la direccion fisica es mayor del RL (Registro Limite), es un error de direccionamiento
                    lanzar_interrupcion(INT_DIR_INVALIDA); 
                    break; 
                }
            } else {
                // En MODO KERNEL, solo se verifica si la direccion fisica es mayor que la memoria
                if (dir_fisica >= TAM_MEMORIA) {
                    lanzar_interrupcion(INT_OVERFLOW); // O INT_DIR_INVALIDA segun prefieras
                    break;
                }
            }

            //  Ejecutar la operacion 
            cpu->SP++; // Actualizar el registro SP
            memoria[dir_fisica] = cpu->AC; // Guardar el AC en la memoria
            
            log_operacion("PSH", cpu->AC, cpu->SP, memoria[dir_fisica]);
            break;
            
        case 26: // pop
            // Verificar Underflow (Pila vacia)
            if (cpu->SP <= 0) {
                lanzar_interrupcion(INT_UNDERFLOW);
                break;
            }

            dir_fisica = cpu->RX + cpu->SP;

            // Verificar si estamos en MODO USUARIO
            if (cpu->PSW.modo == MODO_USUARIO) {

                // Verificar si la direccion fisica es valida
                if (!cpu_verificar_memoria(cpu, dir_fisica)) {
                    lanzar_interrupcion(INT_DIR_INVALIDA);
                    break;
                }
            } else {
                if (dir_fisica >= TAM_MEMORIA) {
                    lanzar_interrupcion(INT_OVERFLOW);
                    break;
                }
            }


            // 3. Ejecutar la operacion
            cpu->AC = memoria[dir_fisica]; // Leemos de la direccion fisica
            cpu->SP--; // Bajamos el puntero
            
            log_operacion("POP", cpu->AC, cpu->SP, cpu->AC);
            break;
            
        case 27: // j - salto incondicional
            operando = cpu_obtener_operando(cpu, inst, memoria);
            if (!interrupcion_pendiente) {
                cpu_saltar(cpu, operando);
                log_operacion("J", 0, operando, cpu->PSW.pc);
            }
            break;
            
        case 28: // sdmap - establecer pista
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede establecer la pista del DMA
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            dma_set_pista(dma, inst.valor);
            log_operacion("SDMAP", 0, inst.valor, 0);
            break;
            
        case 29: // sdmac - establecer cilindro
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede establecer el cilindro del DMA
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            dma_set_cilindro(dma, inst.valor);
            log_operacion("SDMAC", 0, inst.valor, 0);
            break;
            
        case 30: // sdmas - establecer sector
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede establecer el sector del DMA
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            dma_set_sector(dma, inst.valor);
            log_operacion("SDMAS", 0, inst.valor, 0);
            break;
            
        case 31: // sdmaio - establecer operacion (0=Leer, 1=Escribir)
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede establecer la operacion del DMA
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            dma_set_operacion(dma, inst.valor);
            log_operacion("SDMAIO", 0, inst.valor, 0);
            break;
            
        case 32: // sdmam - establecer direccion memoria
            int dir_dma = inst.valor;

            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede establecer la direccion del DMA
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            
            // Configurar el DMA con la direccion
            dma_set_direccion(dma, dir_dma);
            log_operacion("SDMAM", 0, inst.valor, 0);
            break;
            
        case 33: // sdmaon - iniciar DMA
            if (cpu->PSW.modo == MODO_USUARIO) {
                // Un usuario NO puede iniciar el DMA
                lanzar_interrupcion(INT_INST_INVALIDA); 
                break;
            }
            dma_iniciar(dma);
            log_operacion("SDMAON", 0, 0, 0);
            break;
            
        default:
            lanzar_interrupcion(INT_INST_INVALIDA);
            log_error("Instruccion invalida", inst.codigo_op);
            break;
    }
}

 //Guarda los datos del PSW en la RAM
palabra_t cpu_psw_a_palabra(PSW_t psw) {
    return psw.codigo_condicion * 10000000 +     // Pone el CC en el 8vo digito
           psw.modo * 1000000 +                  // Pone el Modo en el 7mo digito
           psw.interrupciones * 100000 +         // Pone las Int en el 6to digito
           psw.pc;                               // Los ultimos 5 digitos son para el PC
}

PSW_t cpu_palabra_a_psw(palabra_t palabra) {
    PSW_t psw;
    psw.codigo_condicion = palabra / 10000000;    // Extrae el digito de la izquierda
    psw.modo = (palabra / 1000000) % 10;          // Aisla el digito del modo
    psw.interrupciones = (palabra / 100000) % 10;  // Aisla el digito de interrupciones
    psw.pc = palabra % 100000;                     // Se queda con los ultimos 5 digitos
    return psw;
}

 //Se usa cuando ocurre una interrupcion, guardamos todo para que el SO pueda retomar
void cpu_salvar_contexto(CPU_t *cpu, palabra_t *memoria) {
    // Determinamos si el SP es relativo (Usuario) o absoluto (Kernel)
    
    int base = cpu->RX;

    // Sube el puntero de pila y guarda el AC
    cpu->SP++;
    int dir_fisica = base + cpu->SP;
    memoria[dir_fisica] = cpu->AC;
    
    // Sube el puntero y guarda el RX
    cpu->SP++;
    dir_fisica = base + cpu->SP;
    memoria[dir_fisica] = cpu->RX;
    
    // Guardar PSW
    cpu->SP++;
    dir_fisica = base + cpu->SP;
    memoria[dir_fisica] = cpu_psw_a_palabra(cpu->PSW);
}

//saca los valores de la pila para que la CPU siga exactamente donde se quedo
void cpu_restaurar_contexto(CPU_t *cpu, palabra_t *memoria) {
    int base = cpu->RX;

    // Recuperar PSW
    int dir_fisica = base + cpu->SP;
    palabra_t psw_raw = memoria[dir_fisica];

    // Recupera y desglosa el PSW, luego baja la pila
    cpu->PSW = cpu_palabra_a_psw(psw_raw);
    cpu->SP--;
    
    // Recuperar RX
    dir_fisica = base + cpu->SP;
    cpu->RX = memoria[dir_fisica];
    cpu->SP--;
    
    // Recuperar AC
    dir_fisica = base + cpu->SP;
    cpu->AC = memoria[dir_fisica];
    cpu->SP--;
}

void cpu_ciclo_instruccion(CPU_t *cpu, palabra_t *memoria, ControladorDMA_t *dma) {
    // Fase de busqueda
    cpu_busqueda(cpu, memoria);

    // Si el fetch lanzo una interrupcion (ej. fuera de limites), abortamos el ciclo
    if (interrupcion_pendiente) {
        return; 
    }
    
    // Fase de decodificacion
    Instruccion_t inst = cpu_decodificar_instruccion(cpu->IR);
    
    // Fase de ejecucion
    cpu_ejecutar(cpu, inst, memoria, dma);
}
