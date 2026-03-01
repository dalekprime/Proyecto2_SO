#include "memory.h"

//Lectura en Memoria
int memory_read(int addr){
    //Acquirir el Bus
    pthread_mutex_lock(&sys.bus_mutex);
    //Verificar Validez de la Direccion
    if(sys.cpu_registers.PSW.operation_mode == 0){
        int eff_addr = addr;
        if(eff_addr < sys.cpu_registers.RB || eff_addr > sys.cpu_registers.RL){
            sys.pending_interrupt = INT_INVALID_ADDR;
            pthread_mutex_unlock(&sys.bus_mutex);
            return -1;
        };
        addr = eff_addr;
    };
    if(addr < 0 || addr >= MEM_SIZE){
        sys.pending_interrupt = INT_INVALID_ADDR;
        pthread_mutex_unlock(&sys.bus_mutex);
        return -1;
    };
    //Busqueda del Dato
    int data = sys.ram[addr];
    //Liberar el Bus
    pthread_mutex_unlock(&sys.bus_mutex);
    return data;
};

//Escritura en Memoria
void memory_write(int addr, int data){
    //Acquirir el Bus
    pthread_mutex_lock(&sys.bus_mutex);
    //Verificar Validez de la Direccion
    if(sys.cpu_registers.PSW.operation_mode == 0){
        int eff_addr = addr;
        if(eff_addr < sys.cpu_registers.RB || eff_addr > sys.cpu_registers.RL){
            sys.pending_interrupt = INT_INVALID_ADDR;
            pthread_mutex_unlock(&sys.bus_mutex);
            return;
        };
        addr = eff_addr;
    };
    if(addr < 0 || addr >= MEM_SIZE){
        sys.pending_interrupt = INT_INVALID_ADDR;
        pthread_mutex_unlock(&sys.bus_mutex);
        return;
    }
    sys.ram[addr] = data;
    //Liberar el Bus
    pthread_mutex_unlock(&sys.bus_mutex);
};