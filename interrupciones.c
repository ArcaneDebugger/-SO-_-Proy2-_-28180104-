#include "interrupciones.h"
#include "cpu.h"
#include "logger.h"
#include <stdio.h>

int interrupcion_pendiente = 0;
int codigo_interrupcion = 0;

void interrupciones_inicializar(VectorInterrupciones_t *vec) {
    int i;
    for (i = 0; i < 9; i++) {
        vec->manejadores[i] = 0; // Direcciones por defecto
    }
    interrupcion_pendiente = 0;
    codigo_interrupcion = 0;
    log_mensaje("Vector de interrupciones inicializado");
}

void lanzar_interrupcion(int codigo) {
    // Verificar que el codigo de interrupcion sea valido
    if (codigo < 0 || codigo > 8) {
        lanzar_interrupcion(INT_COD_INVALIDO);
        return;
    }
    
    interrupcion_pendiente = 1;
    codigo_interrupcion = codigo;
    
    char msg[200];
sprintf(msg, "INTERRUPCION ARROJADA: Codigo %d - %s", 
            codigo, obtener_nombre_interrupcion(codigo));
    log_mensaje(msg);
    printf("Interrupcion: %s\n", msg);
}

const char* obtener_nombre_interrupcion(int codigo) {
    switch(codigo) {
        case INT_COD_SIST_INVALIDO:
            return "Codigo de llamada al sistema invalido";
        case INT_COD_INVALIDO:
            return "Codigo de interrupcion invalido";
        case INT_SYSCALL:
            return "Llamada al sistema";
        case INT_RELOJ:
            return "Interrupcion de reloj";
        case INT_IO_FINALIZADA:
            return "Finalizacion de operacion E/S";
        case INT_INST_INVALIDA:
            return "Instruccion invalida";
        case INT_DIR_INVALIDA:
            return "Direccionamiento invalido";
        case INT_UNDERFLOW:
            return "Underflow";
        case INT_OVERFLOW:
            return "Overflow";
        default:
            return "Interrupcion desconocida";
    }
}

void procesar_interrupcion(CPU_t *cpu, palabra_t *memoria, VectorInterrupciones_t *vec) {
    if (!interrupcion_pendiente) {
        return;
    }
    
    // Verificar si interrupciones estan habilitadas
    if (cpu->PSW.interrupciones == INT_DESHABILITADAS) {
        // Solo algunas interrupciones criticas se procesan
        if (codigo_interrupcion != INT_OVERFLOW &&
            codigo_interrupcion != INT_UNDERFLOW && 
            codigo_interrupcion != INT_DIR_INVALIDA &&
            codigo_interrupcion != INT_INST_INVALIDA) {
            return; // Postponer interrupcion
        }
    }
    
    char msg[200];
    sprintf(msg, "PROCESANDO INTERRUPCION: Codigo %d - %s", 
            codigo_interrupcion, obtener_nombre_interrupcion(codigo_interrupcion));
    log_mensaje(msg);
    
    // Guarda el estado del cpu
    cpu_salvar_contexto(cpu, memoria);
    
    // Cambiar a modo kernel para manejar la interrupcion
    int modo_anterior = cpu->PSW.modo;
    cpu->PSW.modo = MODO_KERNEL;
    
    // Deshabilitar interrupciones durante el manejo
    cpu->PSW.interrupciones = INT_DESHABILITADAS;
    
    // Obtener direccion del manejador de la interrupcion
    int dir_manejador = vec->manejadores[codigo_interrupcion];
    
    if (dir_manejador > 0) {
        cpu->PSW.pc = dir_manejador;
    } else {
        // Manejador por defecto, no hace nada
        sprintf(msg, "No hay manejador para interrupcion %d, continua la ejecucion", 
                codigo_interrupcion);
        log_mensaje(msg);
    }
    
    // Limpiar bandera de interrupcion
    interrupcion_pendiente = 0;
    
    // Restaurar contexto
    cpu_restaurar_contexto(cpu, memoria);
    cpu->PSW.modo = modo_anterior;
    cpu->PSW.interrupciones = INT_HABILITADAS;
}