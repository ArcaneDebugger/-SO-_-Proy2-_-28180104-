#ifndef MEMORIA_H
#define MEMORIA_H

#include "tipos.h"

// Estructura para control de memoria
typedef struct {
    palabra_t datos[TAM_MEMORIA];
    int ocupado[TAM_MEMORIA];
} Memoria_t;

// Inicializa la memoria
void memoria_inicializar(Memoria_t *mem);

// Lee de memoria
palabra_t memoria_leer(Memoria_t *mem, int direccion);

// Escribe en memoria
void memoria_escribir(Memoria_t *mem, int direccion, palabra_t dato);

// Carga programa desde archivo
int memoria_cargar_programa(Memoria_t *mem, const char *archivo, int dir_inicio, int *cant_palabras);

#endif