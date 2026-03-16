#ifndef DISCO_H
#define DISCO_H

#include "tipos.h"

// Definimos el tamaño máximo de código que puede tener un programa.
#define MAX_CODE_SIZE 500

// Estructura que representa un sector del disco.
typedef struct {
    int ocupado;
    char nombre_programa[50];
    palabra_t codigo[MAX_CODE_SIZE];
    int cant_palabras;
} SectorDisco_t;

// Estructura que representa el disco.
typedef struct {
    SectorDisco_t sectores[MAX_PROCESOS];
    int cantidad_programas;
} SimuladorDisco_t;

// Función para inicializar el disco.
void disco_inicializar(SimuladorDisco_t *disco);

// Función para cargar un programa al disco desde un archivo.
int disco_cargar_programa(SimuladorDisco_t *disco, const char *archivo, int *cant_palabras);

// Función para obtener el código de un programa almacenado en disco.
int disco_leer_programa(SimuladorDisco_t *disco, int indice_sector, palabra_t *buffer, int *cant_palabras);

#endif
