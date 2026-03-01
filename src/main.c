#include "main.h"

SYSTEM_STATE sys;

void init(){
    //Iniciar Registros
    sys.cpu_registers.AC = 0;
    sys.cpu_registers.IR = 0;
    sys.cpu_registers.MAR = 0;
    sys.cpu_registers.MDR = 0;
    sys.cpu_registers.RB = OS_MEM_RESERVED;
    sys.cpu_registers.RL = OS_MEM_RESERVED;
    sys.cpu_registers.RX = OS_MEM_RESERVED;
    sys.cpu_registers.SP = OS_MEM_RESERVED;
    //Iniciar Estado
    sys.cpu_registers.PSW.condition_code = 0;
    sys.cpu_registers.PSW.operation_mode = 1; //El SO arranca en Kernel
    sys.cpu_registers.PSW.interruptions_enabled = 0;
    sys.cpu_registers.PSW.pc = 0;
    //Iniciar Parametros
    sys.debug_mode_enabled = 0;
    sys.time = 0;
    sys.pending_interrupt = INT_NONE;
    //Iniciar DMA
    sys.dma_controller.selected_cylinder = 0;
    sys.dma_controller.selected_sector = 0;
    sys.dma_controller.selected_track = 0;
    sys.dma_controller.ram_address = 0;
    sys.dma_controller.status = 0;
    sys.dma_controller.active = false;
    sys.dma_controller.shutdown = false;
    //Carga Codigo de Interrupciones
    sys.ram[99] = 89000000;
    sys.ram[100] = 90000000;
    sys.ram[101] = 91000000;
    sys.ram[102] = 92000000;
    sys.ram[103] = 93000000;
    sys.ram[104] = 94000000;
    sys.ram[105] = 95000000;
    sys.ram[106] = 96000000;
    sys.ram[107] = 97000000;
    sys.ram[108] = 98000000;
    //Iniciar Semaforos
    pthread_mutex_init(&sys.bus_mutex, NULL);
    pthread_mutex_init(&sys.log_mutex, NULL);
    sem_init(&sys.cpu_wakeup, 0, 0); //Inicializar semaforo en 0
}

//Cargador del SO
void start_process(const char* prog_name) {
    extern int prog_count;
    int disk_idx = -1;
    //Buscar en disco
    for (int i = 0; i < MULTIPROGRAMING_GRADE; i++) {
        if (strcmp(disk_reg[i].prog_name, prog_name) == 0 && disk_reg[i].size > 0) {
            disk_idx = i; 
            break;
        }
    }
    extern int prog_count; 
    //Cargar a disco si no esta ya
    if (disk_idx == -1) {
        char filename[128];
        sprintf(filename, "data/%s", prog_name); 
        int size = load_program(filename, prog_name);
        if (size <= 0) return;
        disk_idx = prog_count - 1; 
    }
    //Buscar particion libre
    int mem_block = -1;
    for (int i = 0; i < MULTIPROGRAMING_GRADE; i++) {
        if (!sys.memory_blocks[i]) {
            mem_block = i;
            sys.memory_blocks[i] = true;
            break;
        }
    }
    if (mem_block == -1) {
        printf("SISTEMA >> Error: RAM Llena. No hay particiones libres para '%s'.\n", prog_name);
        return;
    }
    //Mover de Disco a RAM 
    int base_addr = OS_MEM_RESERVED + (mem_block * MEMORY_BLOCK_SIZE);
    int track = disk_reg[disk_idx].track;
    int cyl = disk_reg[disk_idx].cylinder;
    for (int i = 0; i < disk_reg[disk_idx].size; i++) {
        char buffer[TAM_SECTOR];
        read_sector(track, cyl, i, buffer);
        sys.ram[base_addr + i] = atoi(buffer);
    }
    //Asignar BCP
    int pid = -1;
    for (int i = 0; i < MULTIPROGRAMING_GRADE; i++) {
        if (sys.process_table[i].state == TERMINATED || sys.process_table[i].pid == -1) {
            pid = i; break;
        }
    }
    //Configurar BCP
    sys.process_table[pid].pid = pid;
    strcpy(sys.process_table[pid].prog_name, prog_name);
    sys.process_table[pid].state = READY;
    sys.process_table[pid].memory_block_asign = mem_block;
    memset(&sys.process_table[pid].data, 0, sizeof(CPU_REGISTERS));
    sys.process_table[pid].data.RB = base_addr;
    sys.process_table[pid].data.RL = base_addr + MEMORY_BLOCK_SIZE - 1;
    sys.process_table[pid].data.SP = base_addr + disk_reg[disk_idx].size -1;
    sys.process_table[pid].data.RX = sys.process_table[pid].data.SP; 
    sys.process_table[pid].data.PSW.pc = 0; 
    sys.process_table[pid].data.PSW.operation_mode = 0; 
    sys.process_table[pid].data.PSW.interruptions_enabled = 1;
    sys.active_process++;
    //Encolar y Despertar CPU
    sys.ready_queue[sys.ready_tail] = pid;
    sys.ready_tail = (sys.ready_tail + 1) % MULTIPROGRAMING_GRADE;
    printf("SISTEMA >> Proceso '%s' (PID %d) cargado en Particion %d.\n", prog_name, pid, mem_block);
    sem_post(&sys.cpu_wakeup); //Despierta el hilo de la CPU
}

//Interprete de Comandos
void shell() {
    char input[256];
    char *comando;
    char *arg;
    printf("\n=== SISTEMA OPERATIVO INICIADO ===\n");
    printf("Comandos disponibles: ejecutar <prog>, ps, memestat, apagar, reiniciar\n");
    while (1) {
        printf("\nUsuario@Simulador-SO:~$ ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        input[strcspn(input, "\n")] = 0;
        input[strcspn(input, "\r")] = 0;
        if (strlen(input) == 0) continue;
        comando = strtok(input, " ");
        //Comando: apagar
        if (strcmp(comando, "apagar") == 0) {
            printf("Apagando el sistema...\n");
            sys.dma_controller.shutdown = true;
            sem_post(&sys.cpu_wakeup); //Despertar CPU para que detecte el shutdown
            break; 
        }
        //Comando: reiniciar
        else if (strcmp(comando, "reiniciar") == 0) {
            printf("Reiniciando el sistema...\n");
            //Detener ejecucion actual y poner CPU en reposo
            sys.current_pid = -1;
            sys.active_process = 0;
            //Limpiar RAM de usuario
            for(int i = OS_MEM_RESERVED; i < MEM_SIZE; i++){
                sys.ram[i] = 0;
            }
            //Limpiar Tablas de Procesos y Bloques de Memoria
            for(int i = 0; i < MULTIPROGRAMING_GRADE; i++){
                sys.process_table[i].pid = -1;
                sys.process_table[i].state = TERMINATED;
                sys.memory_blocks[i] = false;
                disk_reg[i].size = 0;
            }
            //Reiniciar variables del disco
            extern int prog_count;
            extern int next_track;
            extern int next_cylinder;
            prog_count = 0;
            next_track = 0;
            next_cylinder = 0;
            //Reiniciar colas y tiempo
            sys.ready_head = 0;
            sys.ready_tail = 0;
            sys.waiting_head = 0;
            sys.waiting_tail = 0;
            sys.time = 0;
            write_in_log("SISTEMA >> Reinicio completado por el usuario.");
            printf("SISTEMA >> Reinicio completado con exito. Sistema limpio.\n");
        }
        //Comando: ejecutar
        else if (strcmp(comando, "ejecutar") == 0) {
            arg = strtok(NULL, " ");
            if(arg == NULL) printf("Error: Indica un programa (Ej: ejecutar suma.asm)\n");
            while (arg != NULL) {
                start_process(arg);
                arg = strtok(NULL, " ");
            }
        }
        //Comando: ps
        else if (strcmp(comando, "ps") == 0) {
            printf("\n--- TABLA DE PROCESOS ---\n");
            printf("PID\tESTADO\t\tMEMORIA\t\tPROGRAMA\n");
            int found = 0;
            for (int i = 0; i < MULTIPROGRAMING_GRADE; i++) {
                if (sys.process_table[i].pid != -1 && sys.process_table[i].state != TERMINATED) {
                    char estado[15];
                    if(sys.process_table[i].state == RUNNING) strcpy(estado, "EJECUCION");
                    else if(sys.process_table[i].state == READY) strcpy(estado, "LISTO");
                    else if(sys.process_table[i].state == WAITING) strcpy(estado, "DORMIDO");
                    printf("%d\t%s\tPart %d\t\t%s\n", sys.process_table[i].pid, estado, sys.process_table[i].memory_block_asign, sys.process_table[i].prog_name);
                    found = 1;
                }
            }
            if(!found) printf("No hay procesos activos en este momento.\n");
        }
        //Comando: memestat
        else if (strcmp(comando, "memestat") == 0) {
            printf("\n--- ESTADO DE MEMORIA ---\n");
            int ocupadas = 0;
            for (int i = 0; i < MULTIPROGRAMING_GRADE; i++) {
                printf("Particion %d (Base %d a %d): %s\n", i, OS_MEM_RESERVED + (i * MEMORY_BLOCK_SIZE), OS_MEM_RESERVED + ((i+1) * MEMORY_BLOCK_SIZE) - 1, sys.memory_blocks[i] ? "OCUPADA" : "LIBRE");
                if(sys.memory_blocks[i]) ocupadas++;
            }
            printf("\nUso total de RAM de Usuario: %d%%\n", (ocupadas * 100) / MULTIPROGRAMING_GRADE);
        }
        else {
            printf("Comando no reconocido.\n");
        }
    }
}

int main() {
    init();
    init_disk();
    //Limpiar Tablas del SO
    for(int i = 0; i < MULTIPROGRAMING_GRADE; i++){
        sys.process_table[i].pid = -1;
        sys.process_table[i].state = TERMINATED;
        sys.memory_blocks[i] = false;
        disk_reg[i].size = 0;
    }
    sys.current_pid = -1;
    sys.active_process = 0;
    sys.time_interruption = CLOCK_INTERRUPTION_INTERVAL;
    //Iniciar Hilos de Hardware
    pthread_t cpu, dma;
    pthread_create(&dma, NULL, (void*)dma_loop, NULL);
    sys.dma_controller.dma_id = dma;
    pthread_create(&cpu, NULL, (void*)mainloop, NULL);
    //Entregar el control al usuario
    shell();
    //Limpieza final
    pthread_join(cpu, NULL);
    pthread_join(dma, NULL);
    pthread_mutex_destroy(&sys.bus_mutex);
    pthread_mutex_destroy(&sys.log_mutex);
    sem_destroy(&sys.cpu_wakeup);
    return 0;
}