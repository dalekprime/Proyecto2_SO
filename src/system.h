#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

//Constante del Hardware
#define MEM_SIZE 2000
#define OS_MEM_RESERVED 300
#define WORD_SIZE 8
#define MAGNITUDE_LIMIT 9999999
#define CLOCK_INTERRUPTION_INTERVAL 2
#define MULTIPROGRAMING_GRADE 20
#define MEMORY_BLOCK_SIZE 80

//Codigos de Interrupcion
#define INT_NONE -1 //Default
#define INT_SYSCALL_INVALID 0 //Llamada a Sistema Invalida
#define INT_INVALID_INT 1 //Interrumpio Invalida
#define INT_SYSCALL 2 //LLamada a Sistema
#define INT_TIMER 3 //Reloj
#define INT_IO_END 4 //Finalizacion de I/O
#define INT_INVALID_INSTR 5 //Instruccion Invalida
#define INT_INVALID_ADDR 6 //Direccion Invalida
#define INT_UNDERFLOW 7 //Underflow
#define INT_OVERFLOW 8 //Overflow

//Codigos de Estado
#define NEW 0 //Nuevo Proceso
#define READY 1 //Proceso Listo
#define RUNNING 2 //Proceso en Ejecucion
#define WAITING 3 //Proceso Bloqueado
#define TERMINATED 4 //Proceso Terminado

//Palabra de Estado
typedef struct{
    //0 Cero Flag, 1 Signned Flag, 2 Unsignned Flag, 3 Overflow Flag
    int condition_code; 
    //0 Usuario, 1 Kernel
    bool operation_mode; 
    //0 Disable, 1 Enable
    bool interruptions_enabled; 
    //Dirrecion de la Proxima Instruccion
    int pc; 
}PSW_t;

//Registros del Sistema
typedef struct{
    int AC; //Acumulador para Operaciones Generales
    int MAR; //Direccion de Memoria a Buscar
    int MDR; //Contenido del MAR
    int IR; //Instruccion Actual
    int RB; //Registro Base del Proceso actual
    int RL; //Registro Limite del Proceso Actual
    int RX; //Base de la Pila
    int SP; //Tope de la Pila
    PSW_t PSW; //Estado Del sistema
}CPU_REGISTERS;

//Bloque de Control de Proceso
typedef struct{
    int pid;
    char prog_name[64];
    int state;
    CPU_REGISTERS data;
}BCP;

//Registro de Disco
typedef struct{
    char prog_name[64];
    int track;
    int cylinder;
    int sector;
    int size;
}DISK_DIR;

//Estado del DMA
typedef struct {
    pthread_t dma_id;
    // Registros accesibles por instrucciones 
    int selected_cylinder; 
    int selected_track;    
    int selected_sector;   
    int ram_address;       
    bool io_mode; //0 Input, 1 Output
    int status; //0 Exito, 1 Fallo           
    // Control interno 
    bool active;
    //Fin de Programa
    bool shutdown;
} DMA_CONTROLLER;

//Estado de Sistema
typedef struct{
    //Memoria Principal
    int ram[MEM_SIZE];
    //Registros del Sistema
    CPU_REGISTERS cpu_registers;
    //Controlador del DMA
    DMA_CONTROLLER dma_controller;
    //Para Control del BUS
    pthread_mutex_t bus_mutex;
    //Para Control del Logger
    pthread_mutex_t log_mutex;
    //Reloj
    int time;
    //Tiempo Entre Interrupciones de Reloj
    int time_interruption;
    //Modo del Sistema
    int debug_mode_enabled; //0 Disable, 1 Enable
    //Interrupion
    int pending_interrupt;
    //Tabla de Procesos
    BCP process_table[MULTIPROGRAMING_GRADE];
    //Total de Procesos Activos
    int active_process;
    //PID de Proceso Actual
    int current_pid;
    //Contol de Bloques de Memoria
    bool memory_blocks[MULTIPROGRAMING_GRADE];
    //Cola de Listos
    int ready_queue[MULTIPROGRAMING_GRADE];
    int ready_head;
    int ready_tail;
    //Cola de Bloqueados
    int waiting_queue[MULTIPROGRAMING_GRADE];
    int waiting_head;
    int waiting_tail;
}SYSTEM_STATE;

//Instancia del Sistema
extern SYSTEM_STATE sys;

//Instancia de Registros de Disco
extern DISK_DIR disk_reg[20];

//Funciones de Proposito General
static const int POWERS[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
static inline int sign_to_int(int sm_val, int size){
    int div = POWERS[size-1];
    int sign  = sm_val / div;
    int val = sm_val % div;
    if(sign == 1){
        return -val;
    }
    return val;
};
static inline int int_to_sign(int int_val, int size){
    int div = POWERS[size-1];
    int sign = 0;
    if(int_val < 0){
        sign = 1;
        int_val *= -1;
    }
    if(int_val > div - 1){
        int_val = div - 1;
    };
    return (sign * div) + int_val;
};

int memory_read(int addr);
void memory_write(int addr, int data);
void write_in_log(const char *event);
void check_interruptions();

#endif

