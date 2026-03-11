#ifndef CPU_H
#define CPU_H

#include "tipos.h"
#include "dma.h"

// Inicializa la CPU
void cpu_inicializar(CPU_t *cpu);

// Ciclo de instruccion
void cpu_ciclo_instruccion(CPU_t *cpu, palabra_t *memoria, ControladorDMA_t *dma);

// Fase de busqueda
void cpu_busqueda(CPU_t *cpu, palabra_t *memoria);

// Fase de decodificacion
Instruccion_t cpu_decodificar_instruccion(palabra_t instruccion_raw);

// Fase de ejecucion
void cpu_ejecutar(CPU_t *cpu, Instruccion_t inst, palabra_t *memoria, ControladorDMA_t *dma);

// Calcula direccion efectiva
int cpu_calcular_direccion(CPU_t *cpu, Instruccion_t inst);

// Obtiene operando segun modo de direccionamiento
palabra_t cpu_obtener_operando(CPU_t *cpu, Instruccion_t inst, palabra_t *memoria);

// Salta a la direccion indicada
void cpu_saltar(CPU_t *cpu, int direccion_destino_relativa);

// Verifica proteccion de memoria
int cpu_verificar_memoria(CPU_t *cpu, int direccion);

// Actualiza codigo de condicion
void cpu_actualizar_cc(CPU_t *cpu, palabra_t res);

// Convierte PSW a palabra
palabra_t cpu_psw_a_palabra(PSW_t psw);

// Convierte palabra a PSW
PSW_t cpu_palabra_a_psw(palabra_t palabra);

// Salvaguarda registros en pila
void cpu_salvar_contexto(CPU_t *cpu, palabra_t *memoria);

// Restaura registros desde pila
void cpu_restaurar_contexto(CPU_t *cpu, palabra_t *memoria);

#endif