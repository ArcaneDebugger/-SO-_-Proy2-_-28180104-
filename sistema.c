#include "sistema.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sistema_crear_proceso(Sistema_t *sys, const char *archivo) {
    
    //
    int indice_libre = -1;

    // Buscar un hueco libre en la tabla de procesos (máximo 20)
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (sys->tabla_procesos[i].estado == TERMINADO || sys->tabla_procesos[i].pid == 0) {
            indice_libre = i;
            break;
        }
    }

    if (indice_libre == -1) {
        printf("Error: No se pueden crear más procesos. Límite de %d alcanzado.\n", MAX_PROCESOS);
        return -1;
    }

    // 2. Cargar en disco
    int cant_palabras = 0;
    int sector_disco = disco_cargar_programa(&sys->disco, archivo, &cant_palabras);
    if (sector_disco == -1) {
        printf("Error: Fallo al cargar el programa '%s' en el disco.\n", archivo);
        return -1;
    }

    // 3. Asignar memoria estática (Partición)
    int tam_requerido = cant_palabras + TAM_PILA;
    int dir_base = memoria_asignar_espacio(&sys->memoria, tam_requerido);
    
    if (dir_base == -1) {
        if (tam_requerido > TAM_PARTICION) {
            printf("Error: Programa '%s' muy grande (requiere %d, config limite %d).\n", archivo, tam_requerido, TAM_PARTICION);
        } else {
            printf("Error: No hay particiones libres en memoria para '%s'.\n", archivo);
        }
        return -1;
    }

    // 4. Cargar de disco a memoria
    palabra_t buffer_codigo[MAX_CODE_SIZE];
    disco_leer_programa(&sys->disco, sector_disco, buffer_codigo, &cant_palabras);
    memoria_cargar_desde_buffer(&sys->memoria, buffer_codigo, cant_palabras, dir_base);

    // 5. Inicializar BCP
    BCP_t *nuevo_proceso = &sys->tabla_procesos[indice_libre];
    
    nuevo_proceso->pid = ++sys->contador_pids;
    strncpy(nuevo_proceso->nombre_programa, archivo, 49);
    nuevo_proceso->estado = NUEVO;
    nuevo_proceso->tiempo_inicio = sys->ciclos_reloj;
    nuevo_proceso->base_disco = sector_disco;
    nuevo_proceso->tics_dormido = 0;
    
    // 6. Inicializar contexto de CPU
    memset(&nuevo_proceso->contexto, 0, sizeof(CPU_t));
    nuevo_proceso->contexto.PSW.pc = dir_base;
    nuevo_proceso->contexto.PSW.modo = MODO_USUARIO;
    nuevo_proceso->contexto.PSW.interrupciones = INT_HABILITADAS;
    nuevo_proceso->contexto.RB = dir_base;
    nuevo_proceso->contexto.RX = dir_base + cant_palabras;
    nuevo_proceso->contexto.RL = dir_base + TAM_PARTICION - 1; // Limite fijo tamaño particion
    nuevo_proceso->contexto.SP = 0;

    // 7. Registrar LOG
    sistema_log(nuevo_proceso->pid, -1, NUEVO);

    printf("[SO] Proceso %d ('%s') creado exitosamente. Asignado RAM: %d a %d\n", 
            nuevo_proceso->pid, archivo, dir_base, nuevo_proceso->contexto.RL);

    // Mover a LISTO
    nuevo_proceso->estado = LISTO;
    sistema_log(nuevo_proceso->pid, NUEVO, LISTO);

    return nuevo_proceso->pid;
}

int sistema_planificar_rr(Sistema_t *sys) {
    int pid_saliente = sys->proceso_actual;
    int proximo_indice = -1;
    
    int inicio_busqueda = 0;
    if (pid_saliente != -1) {
        for (int i = 0; i < MAX_PROCESOS; i++) {
            if (sys->tabla_procesos[i].pid == pid_saliente) {
                inicio_busqueda = (i + 1) % MAX_PROCESOS;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_PROCESOS; i++) {
        int idx = (inicio_busqueda + i) % MAX_PROCESOS;
        if (sys->tabla_procesos[idx].estado == LISTO && sys->tabla_procesos[idx].pid != 0) {
            proximo_indice = idx;
            break;
        }
    }
    return proximo_indice;
}

void sistema_despachar(Sistema_t *sys, int proximo_indice) {
    int pid_saliente = sys->proceso_actual;

    // 1. SALVAR CONTEXTO
    if (pid_saliente != -1) {
        for (int i = 0; i < MAX_PROCESOS; i++) {
            if (sys->tabla_procesos[i].pid == pid_saliente && sys->tabla_procesos[i].estado == EJECUCION) {
                sys->tabla_procesos[i].contexto = sys->cpu; 
                sys->tabla_procesos[i].estado = LISTO;
                sistema_log(pid_saliente, EJECUCION, LISTO);
                break;
            }
        }
    }

    // 2. CARGAR CONTEXTO
    if (proximo_indice != -1) {
        BCP_t *p_entrante = &sys->tabla_procesos[proximo_indice];
        
        sys->cpu = p_entrante->contexto;
        sys->proceso_actual = p_entrante->pid;
        sys->contador_quantum = 0; // Reiniciamos quantum
        
        p_entrante->estado = EJECUCION;
        sistema_log(p_entrante->pid, LISTO, EJECUCION);

        if (pid_saliente != -1) {
            char log_msg[200];
            sprintf(log_msg, "Cambio de contexto: Saliente PID = %d, Entrante PID = %d", pid_saliente, p_entrante->pid);
            log_mensaje(log_msg);
        } else {
            char log_msg[200];
            sprintf(log_msg, "Despacho inicial: Entrante PID = %d", p_entrante->pid);
            log_mensaje(log_msg);
        }
    } else {
        sys->proceso_actual = -1;
    }
}

void sistema_planificar(Sistema_t *sys) {
    int prox = sistema_planificar_rr(sys);
    
    // Si no hay nadie LISTO y ya hay un proceso corriendo, simplemente dejarlo seguir
    if (prox == -1 && sys->proceso_actual != -1) {
        char log_msg[200];
        sprintf(log_msg, "QUANTUM AGOTADO: PID %d continua (unico proceso listo)", sys->proceso_actual);
        log_mensaje(log_msg);
        sys->contador_quantum = 0; // Reiniciar quantum para el mismo proceso
        return;
    }
    
    // Si hay alguien LISTO, hacer el cambio de contexto
    if (prox != -1) {
        sistema_despachar(sys, prox);
    }
}

void sistema_log(int pid, Estado_t anterior, Estado_t nuevo) {
    const char* nombres[] = {"NUEVO", "LISTO", "EJECUCION", "DORMIDO", "TERMINADO"};
    
    char buffer[256];
    if (anterior == -1) {
        sprintf(buffer, "[ESTADO] Proceso %d: CREADO -> %s", pid, nombres[nuevo]);
    } else {
        sprintf(buffer, "[ESTADO] Proceso %d: %s -> %s", pid, nombres[anterior], nombres[nuevo]);
    }
    
    log_mensaje(buffer);
}

void sistema_inicializar(Sistema_t *sys) {
    // Inicializar mutex
    pthread_mutex_init(&sys->mutex_bus, NULL);    //Controla quien puede usar el bus de datos. (Mutex)
    pthread_mutex_init(&sys->mutex_memoria, NULL); //Protege el acceso al arreglo de datos de la RAM.
    
    // Inicializar componentes
    cpu_inicializar(&sys->cpu);    //Llama a cpu_inicializar para poner los registros de la CPU en cero
    memoria_inicializar(&sys->memoria);  //Inicializa la memoria
    disco_inicializar(&sys->disco);      // Inicializa cache de disco
    dma_inicializar(&sys->dma, sys->memoria.datos, &sys->mutex_bus);
    interrupciones_inicializar(&sys->vector_int);
    
    // Configurar vector de interrupciones para las llamadas al sistema posteriormente
    // lo haremos cuando tengamos las funciones.
    
    // Inicializar tabla de procesos
    for (int i = 0; i < MAX_PROCESOS; i++) {
        sys->tabla_procesos[i].estado = TERMINADO;
        sys->tabla_procesos[i].pid = 0;
    }
    
    sys->proceso_actual = -1;
    sys->contador_quantum = 0;
    sys->contador_pids = 0;
    
    sys->ejecutando = 0;     //Indica si la maquina esta corriendo 
    sys->ciclos_reloj = 0;
    sys->periodo_reloj = 0;
    
    log_mensaje("Sistema completo inicializado");
}

int hay_procesos_activos(Sistema_t *sys) {
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (sys->tabla_procesos[i].estado != TERMINADO && sys->tabla_procesos[i].pid != 0) {
            return 1;
        }
    }
    return 0;
}

void sistema_iniciar_ejecucion(Sistema_t *sys) {
    sys->ejecutando = 1;
    
    // Al arrancar o reiniciar ejecucion, forzamos la planificacion
    sistema_planificar(sys);
    
    char msg[200];
    sprintf(msg, "Iniciando simulacion");
    log_mensaje(msg);
    printf("\n%s\n\n", msg);
    
    while (sys->ejecutando && hay_procesos_activos(sys)) {
        sistema_ciclo(sys);
    }
    printf("\n[SO] Ejecucion finalizada (Todos los procesos terminaron o sistema detenido)\n\n");
    sys->ejecutando = 0;
}

void sistema_manejar_syscall(Sistema_t *sys) {
    int syscall_code = sys->cpu.AC;
    // La pila crece de RX hacia arriba. El tope es RX + SP.
    int tope_pila = sys->cpu.RX + sys->cpu.SP;
    
    char msg[100];
    sprintf(msg, "Llamada al sistema invocada: Codigo %d", syscall_code);
    log_mensaje(msg);

    switch(syscall_code) {
        case 1: { // termina_prog(estado)
            int estado = sys->memoria.datos[tope_pila]; // Extraer param
            sys->cpu.SP--; // Pop
            
            printf("[SO] Programa %d terminado vía Syscall con estado %d\n", sys->proceso_actual, estado);
            
            // Marcar BCP como terminado, liberar memoria y loguear
            for (int i = 0; i < MAX_PROCESOS; i++) {
                if (sys->tabla_procesos[i].pid == sys->proceso_actual) {
                    sys->tabla_procesos[i].estado = TERMINADO;
                    memoria_liberar_espacio(&sys->memoria, sys->tabla_procesos[i].contexto.RB, sys->tabla_procesos[i].contexto.RL);
                    sistema_log(sys->proceso_actual, EJECUCION, TERMINADO);
                    break;
                }
            }
            sys->proceso_actual = -1;
            sistema_planificar(sys);
            break;
        }
        case 2: { // imprime_pantalla(valor)
            int valor = sys->memoria.datos[tope_pila];
            sys->cpu.SP--; // Pop
            printf("[Programa %d en Consola] -> %d\n", sys->proceso_actual, valor);
            break;
        }
        case 3: { // leer_pantalla()
            int entrada;
            printf("[Programa %d solicita entrada] -> ", sys->proceso_actual);
            scanf("%d", &entrada);
            // vaciar buffer de entrada
            int c; while ((c = getchar()) != '\n' && c != EOF);
            
            // Al retorno, se almacena en AC
            sys->cpu.AC = entrada;
            break;
        }
        case 4: { // Dormir(tics)
            int tics = sys->memoria.datos[tope_pila];
            sys->cpu.SP--; // Pop
            printf("[SO] Programa %d se va a dormir por %d tics\n", sys->proceso_actual, tics);
            
            for (int i = 0; i < MAX_PROCESOS; i++) {
                if (sys->tabla_procesos[i].pid == sys->proceso_actual) {
                    sys->tabla_procesos[i].tics_dormido = tics;
                    sys->tabla_procesos[i].estado = DORMIDO;
                    sistema_log(sys->proceso_actual, EJECUCION, DORMIDO);
                    
                    // Salvar contexto actual para cuando despierte
                    sys->tabla_procesos[i].contexto = sys->cpu;
                    break;
                }
            }
            sys->proceso_actual = -1;
            sistema_planificar(sys);
            break;
        }
        default:
            printf("[SO] Error: Llamada al sistema %d no reconocida.\n", syscall_code);
            log_error("Llamada al sistema no valida", syscall_code);
            break;
    }
}

//Esta funcion encapsula lo que pasa en un ciclo de reloj.
void sistema_ciclo(Sistema_t *sys) {
    
    if (!sys->ejecutando) return;
    
    // Arbitraje del bus para CPU
    pthread_mutex_lock(&sys->mutex_bus);  // La CPU pide permiso exclusivo para usar el bus
    
    // Solo ejecutar instruccion si hay un proceso cargado en la CPU
    if (sys->proceso_actual != -1) {
        cpu_ciclo_instruccion(&sys->cpu, sys->memoria.datos, &sys->dma);
    }
    
    // Procesar interrupciones INMEDIATAMENTE despues de la instruccion
    if (interrupcion_pendiente) {
        // Si ocurre una interrupcion y no hay un manejador cargado en el vector
        if (sys->vector_int.manejadores[codigo_interrupcion] == 0) {
            
            // Caso SVC (Codigo 2): El programa hace una llamada al sistema operativo.
            if (codigo_interrupcion == INT_SYSCALL) {
                sistema_manejar_syscall(sys);
                interrupcion_pendiente = 0;
            }
            
            // Caso Direccionamiento Invalido (Codigo 6): El PC se salio de RL.
            else if (codigo_interrupcion == INT_DIR_INVALIDA) {
                log_error("Violacion de limites de memoria", sys->cpu.PSW.pc);
                printf("\nERROR: Direccionamiento invalido en PID %d. Terminando proceso.\n", sys->proceso_actual);
                
                // Finalizar proceso agresivamente
                for (int i = 0; i < MAX_PROCESOS; i++) {
                    if (sys->tabla_procesos[i].pid == sys->proceso_actual) {
                        sys->tabla_procesos[i].estado = TERMINADO;
                        memoria_liberar_espacio(&sys->memoria, sys->tabla_procesos[i].contexto.RB, sys->tabla_procesos[i].contexto.RL);
                        sistema_log(sys->proceso_actual, EJECUCION, TERMINADO);
                        break;
                    }
                }
                sys->proceso_actual = -1;
                interrupcion_pendiente = 0;
                sistema_planificar(sys);
            }
        }
        
        // Si hay manejador o no es critica, se procesa normalmente 
        if (interrupcion_pendiente) {
            procesar_interrupcion(&sys->cpu, sys->memoria.datos, &sys->vector_int);
        }
    }

    // Despertar procesos dormidos
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (sys->tabla_procesos[i].estado == DORMIDO) {
            sys->tabla_procesos[i].tics_dormido--;
            if (sys->tabla_procesos[i].tics_dormido <= 0) {
                sys->tabla_procesos[i].estado = LISTO;
                sistema_log(sys->tabla_procesos[i].pid, DORMIDO, LISTO);
            }
        }
    }

    // Incrementar contador de ciclos y quantum si hay algo corriendo
    sys->ciclos_reloj++;
    
    if (sys->proceso_actual != -1) {
        sys->contador_quantum++;
        if (sys->contador_quantum >= 2) {
            char log_msg[200];
            sprintf(log_msg, "QUANTUM AGOTADO: Proceso saliente PID = %d", sys->proceso_actual);
            log_mensaje(log_msg);
            sys->contador_quantum = 0;
            sistema_planificar(sys);
        }
    } else {
        // Si no hay proceso actual pero hay listos, forzar planificacion
        sistema_planificar(sys);
    }
    
    if (sys->cpu.PSW.pc >= TAM_MEMORIA || sys->cpu.PSW.pc < 0) {
        if (sys->proceso_actual != -1) {
            printf("\nProceso %d finalizado (PC fuera de rango: %d)\n", sys->proceso_actual, sys->cpu.PSW.pc);
            
            for (int i = 0; i < MAX_PROCESOS; i++) {
                if (sys->tabla_procesos[i].pid == sys->proceso_actual) {
                    sys->tabla_procesos[i].estado = TERMINADO;
                    memoria_liberar_espacio(&sys->memoria, sys->tabla_procesos[i].contexto.RB, sys->tabla_procesos[i].contexto.RL);
                    sistema_log(sys->proceso_actual, EJECUCION, TERMINADO);
                    break;
                }
            }
            sys->proceso_actual = -1;
            sistema_planificar(sys);
        }
    }
    
    // IMPORTANTE: Liberar bus de la CPU luego del ciclo
    pthread_mutex_unlock(&sys->mutex_bus);
}

void sistema_consola(Sistema_t *sys) {

    char comando[256];   // Almacenara la linea completa que el usuario escriba.
        
    while (1) {
        
        printf("sistema> ");
        
        // Leemos el comando del usuario.
        if (fgets(comando, sizeof(comando), stdin) == NULL) break;
        
        // Eliminamos el salto de línea del comando.
        comando[strcspn(comando, "\n")] = 0;
        
        // Volvemos al ciclo si no ingresó nada.
        if (strlen(comando) == 0) continue;
    
        // Extraer el primer token (el comando)
        char *token = strtok(comando, " ");
        if (!token) continue;
    
        // Comando para ejecutar procesos (ejecutar <p1> <p2> ...)
        if (strcmp(token, "ejecutar") == 0) {
            int procesos_creados = 0;
            char *prog = strtok(NULL, " ");
            
            // Loop de extracción de parámetros (todos son programas)
            while (prog != NULL) {
                if (sistema_crear_proceso(sys, prog) != -1) {
                    procesos_creados++;
                }
                prog = strtok(NULL, " ");
            }
            
            if (procesos_creados > 0) {
                sistema_iniciar_ejecucion(sys);
            } else {
                printf("No se crearon procesos validos.\n");
            }
        }

        // Comando para mostrar el contenido de la memoria.
        else if (strcmp(token, "memestat") == 0) {
            int ocupada = 0;
            for (int i = 0; i < TAM_MEMORIA; i++) {
                if (sys->memoria.ocupado[i]) ocupada++;
            }
            float porcentaje = (float)ocupada * 100.0f / TAM_MEMORIA;
            printf("\n--- Estado de la Memoria ---\n");
            printf("Palabras Ocupadas  : %d\n", ocupada);
            printf("Palabras Libres    : %d\n", TAM_MEMORIA - ocupada);
            printf("Porcentaje de Uso  : %.2f%%\n", porcentaje);
            printf("Memoria asignable a procesos desde RAM[%d]\n\n", MEM_SO);
        }

        // Comando para mostrar todos los procesos del sistema.
        else if (strcmp(token, "ps") == 0) {
            printf("\n--- Tabla de Procesos ---\n");
            printf("%-5s | %-12s | %-15s | %-10s\n", "PID", "ESTADO", "PROGRAMA", "% MEM");
            printf("------------------------------------------------------\n");
            const char* nombres_estado[] = {"NUEVO", "LISTO", "EJECUCION", "DORMIDO", "TERMINADO"};
            int encontrados = 0;
            for(int i = 0; i < MAX_PROCESOS; i++) {
                if (sys->tabla_procesos[i].pid != 0) {
                    encontrados++;
                    float pct = 0;
                    if (sys->tabla_procesos[i].estado != TERMINADO && sys->tabla_procesos[i].estado != NUEVO) {
                         int size = sys->tabla_procesos[i].contexto.RL - sys->tabla_procesos[i].contexto.RB + 1;
                         // Calculamos el porcentaje en base a la memoria habilitada para procesos (MEM_USUARIO)
                         pct = (float)size * 100.0f / MEM_USUARIO;
                    }
                    printf("%-5d | %-12s | %-15s | %-5.2f%%\n", 
                           sys->tabla_procesos[i].pid,
                           nombres_estado[sys->tabla_procesos[i].estado],
                           sys->tabla_procesos[i].nombre_programa,
                           pct);
                }
            }
            if (encontrados == 0) {
                printf("No hay procesos en el sistema.\n");
            }
            printf("\n");
        }

        // Comando para apagar el sistema.
        else if (strcmp(token, "apagar") == 0) {
            printf("Apagando el sistema...\n");
            break; 
        }

        // Comando para reiniciar el sistema.
        else if (strcmp(token, "reiniciar") == 0) {
            printf("Reiniciando el sistema...\n");
            sistema_limpiar(sys);
            sistema_inicializar(sys);
        }

        // Comando de ayuda para conocer todos los comandos.
        else if (strcmp(token, "ayuda") == 0) {
            printf("\n");
            printf(" %-20s | %s\n", "ejecutar <p1> <pn...>", "Carga y ejecuta uno o varios programas en paralelo.");
            printf(" %-20s | %s\n", "memestat", "Muestra el estado de la Memoria Principal y su \% de uso.");
            printf(" %-20s | %s\n", "ps", "Muestra la tabla de Procesos Activos (PID, Estado, RAM).");
            printf(" %-20s | %s\n", "reiniciar", "Limpia la memoria y reinicia el simulador desde cero.");
            printf(" %-20s | %s\n", "apagar", "Finaliza la consola y apaga el Sistema Operativo.");
            printf(" %-20s | %s\n", "ayuda", "Muestra este menú de opciones.");
            printf("\n");
        }

        // Si se detecta un comando inválido.
        else {
            printf("Comando '%s' no reconocido. Escribe 'ayuda' para ver comandos disponibles.\n", token);
        }
    }
}

void sistema_limpiar(Sistema_t *sys) {
    dma_terminar(&sys->dma);
    pthread_mutex_destroy(&sys->mutex_bus);
    pthread_mutex_destroy(&sys->mutex_memoria);
    log_mensaje("Sistema finalizado correctamente");
}