#ifndef TIPOS_H
#define TIPOS_H

#include <stdint.h>

// Tamaño de palabra: 8 digitos decimales
#define TAM_PALABRA 8
#define TAM_PILA 50 // Tamanio reservado para la pila
#define TAM_MEMORIA 2000 // Tamanio de la memoria ram es de 2000 posiciones
#define MEM_SO 300       //300 posiciones reservadas para el SO
#define MEM_USUARIO (TAM_MEMORIA - MEM_SO)    // Posiciones disponibles para el usuario

// Cantidad máxima de Procesos en la memoria
#define MAX_PROCESOS 20
#define TAM_PARTICION (MEM_USUARIO / MAX_PROCESOS) // 1700 / 20 = 85 particiones estáticas

// El procesador tiene dos modos de ejecucion:
#define MODO_USUARIO 0
#define MODO_KERNEL 1

// Codigos de interrupcion
#define INT_COD_SIST_INVALIDO 0
#define INT_COD_INVALIDO 1
#define INT_SYSCALL 2
#define INT_RELOJ 3
#define INT_IO_FINALIZADA 4
#define INT_INST_INVALIDA 5
#define INT_DIR_INVALIDA 6
#define INT_UNDERFLOW 7
#define INT_OVERFLOW 8

// Estados de interrupciones
#define INT_DESHABILITADAS 0
#define INT_HABILITADAS 1

// Tipos de direccionamiento
#define DIR_DIRECTO 0      // Los ultimos 5 digitos referencian a la posicion de memoria donde esta el dato
#define DIR_INMEDIATO 1    // Los ultimos 5 digitos son el dato
#define DIR_INDEXADO 2     // Suma del contenido actual del AC + el valor de la instruccion, va a esa direccion y el contenido lo guarda en AC
 
// Codigos de condicion: Muestran el res de la operacion aritmetica mas reciente y se ve reflejado en el PSW
#define CC_IGUAL 0
#define CC_MENOR 1
#define CC_MAYOR 2
#define CC_OVERFLOW 3

// Operaciones DMA
#define DMA_LEER 0
#define DMA_ESCRIBIR 1

// Estado DMA
#define DMA_EXITO 0
#define DMA_ERROR 1

// Dimensiones del disco
#define DISCO_PISTAS 10
#define DISCO_CILINDROS 10
#define DISCO_SECTORES 100
#define TAM_SECTOR 9

// Tipo para representar una palabra de 8 digitos
typedef int32_t palabra_t;

// Registro PSW
typedef struct {
    int codigo_condicion;  // 1 digito: res de las operaciones aritmeticas
    int modo;              // 1 digito: 0 es modo usuario y 1 modo privilegiado 
    int interrupciones;    // 1 digito: 0 si estan desabilitadas y 1 si estan habilitadas
    int pc;                // 5 digitos: direccion a la proxima instruccion a leer
} PSW_t;

// Estados del proceso
typedef enum {
    NUEVO,
    LISTO,
    EJECUCION,
    DORMIDO,
    TERMINADO
} Estado_t;

// Estructura de la CPU
typedef struct {
    palabra_t AC;       // Acumulador
    palabra_t MAR;      // Memory Address Register
    palabra_t MDR;      // Memory Data Register
    palabra_t IR;       // Instruction Register
    palabra_t RB;       // Registro Base
    palabra_t RL;       // Registro Limite
    palabra_t RX;       // Registro base de pila
    palabra_t SP;       // Apuntador de pila
    PSW_t PSW;          // Palabra de estado del sistema
} CPU_t;

// Estructura del BCP
typedef struct {
    int pid;                // Identificación del proceso
    char nombre_programa[50]; // Para listar en ps y cache de disco
    Estado_t estado;        // Estado actual
    CPU_t contexto;         // Copia fiel de los registros cuando el proceso no está en CPU
    int tiempo_inicio;      // Para estadísticas y logs
    uint32_t base_disco;    // Dirección donde reside en el disco duro
    int tics_dormido;       // Tics restantes para despertar
    int tamano_real;        // Cantidad de palabras reales (codigo + pila)
} BCP_t;

// Estructura del DMA
typedef struct {
    int pista;
    int cilindro;
    int sector;
    int operacion;      // Leer o escribir
    int dir_memoria;    // Direccion de memoria
    int estado;         // Estado de la operacion
    int activo;         // Si esta trabajando
} DMA_t;

// Estructura del disco
typedef struct {
    char datos[DISCO_PISTAS][DISCO_CILINDROS][DISCO_SECTORES][TAM_SECTOR];
} Disco_t;

// Estructura de la instruccion decodificada
typedef struct {
    int codigo_op;          // 2 digitos
    int direccionamiento;   // 1 digito
    int valor;              // 5 digitos
} Instruccion_t;

#endif
