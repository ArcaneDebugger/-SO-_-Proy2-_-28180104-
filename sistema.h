#ifndef SISTEMA_H
#define SISTEMA_H

#include "tipos.h"
#include "cpu.h"
#include "memoria.h"
#include "dma.h"
#include "interrupciones.h"
#include "disco.h"
#include <pthread.h>

// Estructura principal del sistema.
typedef struct {
    CPU_t cpu;
    Memoria_t memoria;
    SimuladorDisco_t disco;
    ControladorDMA_t dma;
    VectorInterrupciones_t vector_int;
    pthread_mutex_t mutex_bus;
    pthread_mutex_t mutex_memoria;
    BCP_t tabla_procesos[MAX_PROCESOS];

    int proceso_actual;  
    int contador_quantum;
    int contador_pids;
    int ejecutando;
    int ciclos_reloj;
    int periodo_reloj;
    int pico_memoria;
} Sistema_t;

// Función para crear un proceso.
int sistema_crear_proceso(Sistema_t *sys, const char *archivo);

// Función para planificar procesos.
void sistema_planificar(Sistema_t *sys);

// Función para registrar los cambios en el archivo .LOG.
void sistema_log(int pid, Estado_t anterior, Estado_t nuevo);

// Función para inicializar el sistema.
void sistema_inicializar(Sistema_t *sys);

// Función para iniciar la ejecución.
void sistema_iniciar_ejecucion(Sistema_t *sys);

// Función para el ciclo principal de ejecución.
void sistema_ciclo(Sistema_t *sys);

// Función para limpiar los recursos del sistema.
void sistema_limpiar(Sistema_t *sys);

// Función para la consola interactiva.
void sistema_consola(Sistema_t *sys);

#endif