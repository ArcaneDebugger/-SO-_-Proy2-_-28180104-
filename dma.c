#include "dma.h"
#include "interrupciones.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void dma_inicializar(ControladorDMA_t *controlador_dma, palabra_t *memoria, pthread_mutex_t *mutex_bus) {
    //Inicializacion de registros 
    controlador_dma->dma.pista = 0;
    controlador_dma->dma.cilindro = 0;
    controlador_dma->dma.sector = 0;
    controlador_dma->dma.operacion = DMA_LEER;           //Al principio leer
    controlador_dma->dma.dir_memoria = 0;
    controlador_dma->dma.estado = DMA_EXITO;             //Estado inicial
    controlador_dma->dma.activo = 0;
    controlador_dma->memoria = memoria;                  //Guarda la direccion de memoria dentro de la estructura del DMA
    controlador_dma->mutex_bus = mutex_bus;
    controlador_dma->ejecutando = 0;
    
    // Simula un disco duro nuevo 
    memset(&controlador_dma->disco, 0, sizeof(Disco_t));
    
    log_mensaje("DMA y Disco inicializados");   //Escribe en el archivo de registro que el componente se inicio correctamente.
}

//Esta funcion se encarga de seleccionar en que anillo del disco se va a leer o escribir.
void dma_set_pista(ControladorDMA_t *controlador_dma, int pista) {
    controlador_dma->dma.pista = pista;
    char msg[100];
    sprintf(msg, "DMA: Pista establecida = %d", pista);
    log_mensaje(msg);
}
//Esta funcion selecciona el cilindro
void dma_set_cilindro(ControladorDMA_t *controlador_dma, int cilindro) {
    controlador_dma->dma.cilindro = cilindro;            //Guarda el valor del cilindro en la estructura interna.
    char msg[100];
    sprintf(msg, "DMA: Cilindro establecido = %d", cilindro);  
    log_mensaje(msg);
}
//Esta funcion especifica dentro de la pista donde esta el dato.
void dma_set_sector(ControladorDMA_t *controlador_dma, int sector) {
    controlador_dma->dma.sector = sector;
    char msg[100];
    sprintf(msg, "DMA: Sector establecido = %d", sector);
    log_mensaje(msg);
}
//Indica al DMA si debe sacar datos del disco o escribir en el 
void dma_set_operacion(ControladorDMA_t *controlador_dma, int operacion) {
    controlador_dma->dma.operacion = operacion;
    char msg[100];
    sprintf(msg, "DMA: Operacion establecida = %s", 
            operacion == DMA_LEER ? "LEER" : "ESCRIBIR");
    log_mensaje(msg);
}
//Guarda la direccion de memoria fisica donde se va a realizar la transferencia.
void dma_set_direccion(ControladorDMA_t *controlador_dma, int direccion) {
    controlador_dma->dma.dir_memoria = direccion;
    char msg[100];
    sprintf(msg, "DMA: Direccion memoria = %d", direccion);
    log_mensaje(msg);
}

void* dma_thread_func(void *arg) {
    ControladorDMA_t *controlador_dma = (ControladorDMA_t*)arg;
    
    log_mensaje("DMA: Iniciando operacion de E/S");
    
    // Validar parametros
    if (controlador_dma->dma.pista >= DISCO_PISTAS || 
        controlador_dma->dma.cilindro >= DISCO_CILINDROS ||
        controlador_dma->dma.sector >= DISCO_SECTORES) {
        controlador_dma->dma.estado = DMA_ERROR;
        log_error("DMA: Parametros de disco invalidos", 0);
        lanzar_interrupcion(INT_IO_FINALIZADA);
        controlador_dma->dma.activo = 0;
        return NULL;
    }
    
    // Simular tiempo de acceso a disco
    usleep(100000); // 100ms
    
    // Arbitraje del bus
    pthread_mutex_lock(controlador_dma->mutex_bus);
    
    if (controlador_dma->dma.operacion == DMA_LEER) {
        // Leer del disco a memoria
        char *sector_data = controlador_dma->disco.datos[controlador_dma->dma.pista]
                                             [controlador_dma->dma.cilindro]
                                             [controlador_dma->dma.sector];
        
        palabra_t dato;
        sscanf(sector_data, "%d", &dato);

        // Guardar en memoria RAM el dato leido del disco
        controlador_dma->memoria[controlador_dma->dma.dir_memoria] = dato;
        
        log_mensaje("DMA: Lectura de disco completada");
    } else {
        // Extrae el dato de memoria RAM y lo escribe en el disco
        palabra_t dato = controlador_dma->memoria[controlador_dma->dma.dir_memoria];

        // Escribir en el disco
        sprintf(controlador_dma->disco.datos[controlador_dma->dma.pista]
                                 [controlador_dma->dma.cilindro]
                                 [controlador_dma->dma.sector], 
                "%08d", dato);
        
        log_mensaje("DMA: Escritura a disco completada");
    }
    
    pthread_mutex_unlock(controlador_dma->mutex_bus);
    
    // Operacion exitosa
    controlador_dma->dma.estado = DMA_EXITO;
    controlador_dma->dma.activo = 0;
    
    // Lanzar interrupcion de finalizacion
    lanzar_interrupcion(INT_IO_FINALIZADA);
    
    return NULL;
}

void dma_iniciar(ControladorDMA_t *controlador_dma) {
    if (controlador_dma->dma.activo) {
        log_error("DMA ya esta en operacion", 0);
        return;
    }
    
    controlador_dma->dma.activo = 1;
    controlador_dma->ejecutando = 1;
    
    // Crear thread para la operacion DMA
    if (pthread_create(&controlador_dma->thread, NULL, dma_thread_func, controlador_dma) != 0) {
        log_error("Error al crear thread DMA", 0);
        controlador_dma->dma.activo = 0;
        controlador_dma->ejecutando = 0;
        return;
    }
    
    log_mensaje("DMA: Thread de E/S creado");
}

void dma_terminar(ControladorDMA_t *controlador_dma) {
    if (controlador_dma->ejecutando) {
        pthread_join(controlador_dma->thread, NULL);
        controlador_dma->ejecutando = 0;
    }
}