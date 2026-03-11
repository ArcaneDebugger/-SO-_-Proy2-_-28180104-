#ifndef DMA_H
#define DMA_H

#include "tipos.h"
#include <pthread.h>

// Estructura del controlador DMA
typedef struct {
    DMA_t dma;
    Disco_t disco;
    palabra_t *memoria;
    pthread_mutex_t *mutex_bus;
    pthread_t thread;
    int ejecutando;
} ControladorDMA_t;

// Inicializa el DMA
void dma_inicializar(ControladorDMA_t *ctrl, palabra_t *memoria, pthread_mutex_t *mutex_bus);

// Establece parametros del DMA
void dma_set_pista(ControladorDMA_t *ctrl, int pista);
void dma_set_cilindro(ControladorDMA_t *ctrl, int cilindro);
void dma_set_sector(ControladorDMA_t *ctrl, int sector);
void dma_set_operacion(ControladorDMA_t *ctrl, int operacion);
void dma_set_direccion(ControladorDMA_t *ctrl, int direccion);

// Inicia operacion DMA
void dma_iniciar(ControladorDMA_t *ctrl);

// Funcion del thread DMA
void* dma_thread_func(void *arg);

// Limpia recursos
void dma_terminar(ControladorDMA_t *ctrl);

#endif