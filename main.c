#include "sistema.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

int main() {

    Sistema_t sistema;
    
    // Inicializar logger
    log_inicializar();
    
    // Inicializar sistema
    sistema_inicializar(&sistema);
    
    // Lanzar consola interactiva
    sistema_consola(&sistema);
    
    // Limpiar recursos
    sistema_limpiar(&sistema);
    
    log_close();
    
    return 0;
}