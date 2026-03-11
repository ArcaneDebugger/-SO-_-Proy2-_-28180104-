#include "memoria.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>

void memoria_inicializar(Memoria_t *mem) {
    int i;

    // Pone toda la memoria en 0
    for (i = 0; i < TAM_MEMORIA; i++) {
        mem->datos[i] = 0;
        mem->ocupado[i] = 0;
    }
    
    // Marca la zona como area reservada para el Sistema Operativo
    for (i = 0; i < MEM_SO; i++) {
        mem->ocupado[i] = 1;
    }
    
    log_mensaje("Memoria inicializada");
}

palabra_t memoria_leer(Memoria_t *mem, int direccion) {
    if (direccion < 0 || direccion >= TAM_MEMORIA) {
        log_error("Direccion de memoria invalida en lectura", direccion);
        return 0;
    }
    return mem->datos[direccion];
}

void memoria_escribir(Memoria_t *mem, int direccion, palabra_t dato) {
    if (direccion < 0 || direccion >= TAM_MEMORIA) {
        log_error("Direccion de memoria invalida en escritura", direccion);
        return;
    }
    mem->datos[direccion] = dato;
}

int memoria_cargar_desde_buffer(Memoria_t *mem, const palabra_t *buffer, int cant_palabras, int dir_inicio) {
    if (dir_inicio + cant_palabras > TAM_MEMORIA) {
        log_error("Fallo al escribir en memoria: supera el limite", dir_inicio);
        return -1;
    }

    for (int i = 0; i < cant_palabras; i++) {
        mem->datos[dir_inicio + i] = buffer[i];
        mem->ocupado[dir_inicio + i] = 1;
        
        char msg[200];
        sprintf(msg, "Cargado en RAM[%d]: %08d", dir_inicio + i, buffer[i]);
        log_mensaje(msg);
    }
    
    return dir_inicio;
}

int memoria_asignar_espacio(Memoria_t *mem, int tam_requerido) {
    // Si el tamano excede la particion estatica, falla directamente
    if (tam_requerido > TAM_PARTICION) {
        return -1;
    }

    // Busqueda de una particion libre (Particionamiento estatico puro)
    for (int p = 0; p < MAX_PROCESOS; p++) {
        int inicio = MEM_SO + (p * TAM_PARTICION);
        
        // Verificamos el bloque buscando la primera partición que no esté ocupada
        if (mem->ocupado[inicio] == 0) {
            
            // Se encontró partición libre. Se marca toda la partición estática como ocupada.
            for (int i = inicio; i < inicio + TAM_PARTICION; i++) {
                mem->ocupado[i] = 1;
            }
            
            char msg[200];
            sprintf(msg, "Memoria asignada (Particion %d): RAM[%d] a RAM[%d]", p + 1, inicio, inicio + TAM_PARTICION - 1);
            log_mensaje(msg);
            
            return inicio; // Retorna la base física
        }
    }
    return -1; // No hay particiones libres
}

void memoria_liberar_espacio(Memoria_t *mem, int base, int limite) {
    if (base < MEM_SO || limite >= TAM_MEMORIA || base > limite) return;
    for (int i = base; i <= limite; i++) {
        mem->ocupado[i] = 0;
        mem->datos[i] = 0;
    }
    char msg[200];
    sprintf(msg, "Memoria liberada: RAM[%d] a RAM[%d]", base, limite);
    log_mensaje(msg);
}