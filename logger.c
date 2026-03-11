#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

static FILE *log_file = NULL;

void log_inicializar(void) {
    log_file = fopen("sistema.log", "w");
    if (!log_file) {
        fprintf(stderr, "Error: No se pudo crear archivo de log\n");
        return;
    }
    
    time_t ahora = time(NULL);
    fprintf(log_file, "=== Sistema iniciado: %s", ctime(&ahora));
    fprintf(log_file, "========================================\n\n");
    fflush(log_file);
}

void log_close(void) {
    if (log_file) {
        time_t ahora = time(NULL);
        fprintf(log_file, "\n========================================\n");
        fprintf(log_file, "=== Sistema finalizado: %s", ctime(&ahora));
        fclose(log_file);
        log_file = NULL;
    }
}

void log_mensaje(const char *mensaje) {
    if (!log_file) return;
    
    time_t ahora = time(NULL);
    char timestamp[26];
    strcpy(timestamp, ctime(&ahora));
    timestamp[strlen(timestamp)-1] = '\0'; // Eliminar \n
    
    fprintf(log_file, "[%s] %s\n", timestamp, mensaje);
    fflush(log_file);
}

void log_operacion(const char *op, palabra_t op1, palabra_t op2, palabra_t res) {
    if (!log_file) return;
    
    char buffer[256];
    sprintf(buffer, "OPERACION: %s | Op1=%d, Op2=%d, res=%d", 
            op, op1, op2, res);
    log_mensaje(buffer);
}

void log_error(const char *mensaje, int codigo) {
    if (!log_file) return;
    
    char buffer[256];
    sprintf(buffer, "ERROR: %s (Codigo: %d)", mensaje, codigo);
    log_mensaje(buffer);
}

void log_interrupcion(const char *mensaje) {
    if (!log_file) return;
    
    fprintf(log_file, "\n");
    fprintf(log_file, "************************************\n");
    fprintf(log_file, "*** %s ***\n", mensaje);
    fprintf(log_file, "************************************\n");
    fprintf(log_file, "\n");
    fflush(log_file);
}