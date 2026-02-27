#include "dma.h"

// Bucle principal simulando el circuito del DMA
void* dma_loop() {
    Sector buffer; // Buffer interno del DMA (tamaño 512 bytes)
    while (1) {
        //Espera a iniciar
        while (!sys.dma_controller.active) {
            if(sys.dma_controller.shutdown == 1){
                break;
            };
            usleep(1000);
        };
        if(sys.dma_controller.shutdown == 1){
                break;
            };
        write_in_log("Iniciando Operacion I/O..");
        //Bloqueamos el bus para que la CPU no toque la RAM mientras transferimos
        pthread_mutex_lock(&sys.bus_mutex);
        //Copiar parámetros a variables locales
        int track = sys.dma_controller.selected_track;
        int cyl = sys.dma_controller.selected_cylinder;
        int sec = sys.dma_controller.selected_sector;
        int ram_addr = sys.dma_controller.ram_address;
        bool mode = sys.dma_controller.io_mode; // 0=Leer (Disk->Mem), 1=Escribir (Mem->Disk)
        int result_status = 0;
        if (mode == 0) { 
            // MODO LECTURA: DISCO -> RAM 
            //Leer del Disco al Buffer interno
            memset(&buffer, 0, sizeof(Sector));
            read_sector(track, cyl, sec, (char*)buffer.data);
            //Escribir en RAM
            if (ram_addr < MEM_SIZE && ram_addr >= 0) {
                sys.ram[ram_addr] = atoi((char*)buffer.data);
            } else {
                result_status = 1;
            }
            write_in_log("Operacion DISK -> RAM..");
        } else { 
            //MODO ESCRITURA: RAM -> DISCO
            //Leer de RAM
            memset(&buffer, 0, sizeof(Sector));
            if ((ram_addr) < MEM_SIZE && (ram_addr) >= 0) {
                // Convertimos el entero de RAM a texto para guardarlo en el disco
                sprintf((char*)buffer.data, "%d", sys.ram[ram_addr]);
            } else {
                result_status = 1;
            }
            write_in_log("Operacion RAM -> DISk..");
            //Escribir en el Disco
            write_sector(track, cyl, sec, (char*)buffer.data);
        }
        //FINALIZACIÓN Y RETORNO DE MANDO
        sys.dma_controller.active = false;
        sys.dma_controller.status = result_status; // 0=exito, 1=error
        //GENERAR INTERRUPCIÓN
        sys.pending_interrupt = INT_IO_END;
        //LIBERAR EL BUS
        pthread_mutex_unlock(&sys.bus_mutex);
        if(sys.dma_controller.shutdown == 1){
            break;
        };
    }
    return NULL;
}