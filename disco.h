#ifndef DISCO_H
#define DISCO_H

#include "tipos.h"

// Abstracción simplificada del disco para almacenar los programas
#define MAX_CODE_SIZE 500

typedef struct {
    int ocupado;
    char nombre_programa[50];
    palabra_t codigo[MAX_CODE_SIZE];
    int cant_palabras;
} SectorDisco_t;

typedef struct {
    SectorDisco_t sectores[MAX_PROCESOS];
    int cantidad_programas;
} SimuladorDisco_t;

// Inicializa el disco
void disco_inicializar(SimuladorDisco_t *disco);

// Carga un programa al disco desde un archivo .prog (si no existe ya)
// Retorna el índice del sector donde se cargó, o -1 si hubo error
int disco_cargar_programa(SimuladorDisco_t *disco, const char *archivo, int *cant_palabras);

// Obtiene el código de un programa almacenado en disco
// Retorna 0 si tuvo éxito, -1 en caso contrario
int disco_leer_programa(SimuladorDisco_t *disco, int indice_sector, palabra_t *buffer, int *cant_palabras);

#endif
