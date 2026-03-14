#include "disco.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void disco_inicializar(SimuladorDisco_t *disco) {
    disco->cantidad_programas = 0;
    for (int i = 0; i < MAX_PROCESOS; i++) {
        disco->sectores[i].ocupado = 0;
        disco->sectores[i].cant_palabras = 0;
        memset(disco->sectores[i].nombre_programa, 0, 50);
    }
    log_mensaje("Disco inicializado");
}

int disco_cargar_programa(SimuladorDisco_t *disco, const char *archivo, int *cant_palabras) {
    // Verificar si ya está en caché del disco
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (disco->sectores[i].ocupado && strcmp(disco->sectores[i].nombre_programa, archivo) == 0) {
            char msg[200];
            sprintf(msg, "Programa %s cargado desde cache de disco.", archivo);
            log_mensaje(msg);
            if (cant_palabras) *cant_palabras = disco->sectores[i].cant_palabras;
            return i;
        }
    }

    // Buscar espacio libre
    int indice_libre = -1;
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (!disco->sectores[i].ocupado) {
            indice_libre = i;
            break;
        }
    }

    if (indice_libre == -1) {
        log_error("Disco lleno, no se pueden almacenar mas programas", 0);
        return -1;
    }

    // Leer el archivo local
    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        log_error("No se pudo abrir archivo para cargar al disco", 0);
        return -1;
    }

    SectorDisco_t *sector = &disco->sectores[indice_libre];
    strncpy(sector->nombre_programa, archivo, 49);
    sector->cant_palabras = 0;

    char linea[100];
    int en_codigo = 0;

    while (fgets(linea, sizeof(linea), fp)) {
        // 1. Limpiar espacios y caracteres de control al inicio
        char *ptr = linea;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        
        // 2. Ignorar lineas vacias o comentarios
        if (*ptr == '\0' || strncmp(ptr, "//", 2) == 0) continue;

        // 3. Procesar directivas
        if (strncmp(ptr, ".NombreProg", 11) == 0) {
            en_codigo = 1;
            continue;
        } 
        
        if (strncmp(ptr, "_start", 6) == 0 || strncmp(ptr, ".NumeroPalabras", 15) == 0) {
            continue;
        }

        // 4. Si estamos en zona de codigo y empieza por numero, cargarlo
        if (en_codigo && isdigit((unsigned char)ptr[0])) {
            palabra_t instruccion = (palabra_t)strtol(ptr, NULL, 10);

            if (sector->cant_palabras >= MAX_CODE_SIZE) {
                log_error("Programa excede el limite del sector de disco", MAX_CODE_SIZE);
                fclose(fp);
                sector->ocupado = 0;
                return -1;
            }

            sector->codigo[sector->cant_palabras++] = instruccion;
        }
    }

    fclose(fp);
    sector->ocupado = 1;
    disco->cantidad_programas++;

    char msg[256];
    sprintf(msg, "Programa '%s' cargado al disco (Sector: %d, Palabras: %d)", archivo, indice_libre, sector->cant_palabras);
    log_mensaje(msg);

    if (cant_palabras) *cant_palabras = sector->cant_palabras;
    return indice_libre;
}

int disco_leer_programa(SimuladorDisco_t *disco, int indice_sector, palabra_t *buffer, int *cant_palabras) {
    if (indice_sector < 0 || indice_sector >= MAX_PROCESOS || !disco->sectores[indice_sector].ocupado) {
        return -1;
    }

    SectorDisco_t *sector = &disco->sectores[indice_sector];
    for (int i = 0; i < sector->cant_palabras; i++) {
        buffer[i] = sector->codigo[i];
    }
    
    if (cant_palabras) *cant_palabras = sector->cant_palabras;
    return 0;
}
