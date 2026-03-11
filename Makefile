# Makefile para Arquitectura Virtual

CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = sistema
OBJS = main.o sistema.o cpu.o memoria.o disco.o dma.o interrupciones.o logger.o

# Regla principal
all: $(TARGET)

# Enlazar objetos
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compilar archivos objeto
main.o: main.c sistema.h logger.h
	$(CC) $(CFLAGS) -c main.c

sistema.o: sistema.c sistema.h cpu.h memoria.h disco.h dma.h interrupciones.h logger.h tipos.h
	$(CC) $(CFLAGS) -c sistema.c

cpu.o: cpu.c cpu.h interrupciones.h logger.h tipos.h
	$(CC) $(CFLAGS) -c cpu.c

memoria.o: memoria.c memoria.h logger.h tipos.h
	$(CC) $(CFLAGS) -c memoria.c

disco.o: disco.c disco.h logger.h tipos.h
	$(CC) $(CFLAGS) -c disco.c

dma.o: dma.c dma.h interrupciones.h logger.h tipos.h
	$(CC) $(CFLAGS) -c dma.c

interrupciones.o: interrupciones.c interrupciones.h cpu.h logger.h tipos.h
	$(CC) $(CFLAGS) -c interrupciones.c

logger.o: logger.c logger.h tipos.h
	$(CC) $(CFLAGS) -c logger.c

# Limpiar archivos generados
clean:
	rm -f $(OBJS) $(TARGET) sistema.log

# Reconstruir todo
rebuild: clean all

.PHONY: all clean rebuild