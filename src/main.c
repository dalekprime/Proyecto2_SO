#include "main.h"

SYSTEM_STATE sys;

void init(){
    //Iniciar Registros
    sys.cpu_registers.AC = 0;
    sys.cpu_registers.IR = 0;
    sys.cpu_registers.MAR = 0;
    sys.cpu_registers.MDR = 0;
    sys.cpu_registers.RB = 300;
    sys.cpu_registers.RL = 300;
    sys.cpu_registers.RX = 300;
    sys.cpu_registers.SP = 300;
    //Iniciar Estado
    sys.cpu_registers.PSW.condition_code = 0;
    sys.cpu_registers.PSW.operation_mode = 0;
    sys.cpu_registers.PSW.interruptions_enabled = 1;
    sys.cpu_registers.PSW.pc = 0;
    //Inciar Parametros
    sys.debug_mode_enabled = 0;
    sys.time = 0;
    sys.pending_interrupt = INT_NONE;
    //Inciar DMA
    sys.dma_controller.selected_cylinder = 0;
    sys.dma_controller.selected_sector = 0;
    sys.dma_controller.selected_track = 0;
    sys.dma_controller.ram_address = 0;
    sys.dma_controller.status= 0;
    sys.dma_controller.active = false;
    sys.dma_controller.shutdown = false;
    //Carga Codigo de Interrumpciones
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
    //Iniciar Semaforos (Solo una vez al arranque)
    pthread_mutex_init(&sys.bus_mutex, NULL);
    pthread_mutex_init(&sys.log_mutex, NULL);
};

void startProgram(){
    char dir[128];
    char real_dir[256];
    printf("Ingrese Nombre del Programa\n");
    scanf("%s", dir);
    sprintf(real_dir, "data/%s", dir);
    int prog_size = load_program(real_dir, sys.cpu_registers.RB);
    pthread_t cpu, dma;
    if (prog_size > 0) {
        int code_end = sys.cpu_registers.RB + prog_size;
        sys.cpu_registers.SP = code_end;
        sys.cpu_registers.RX = sys.cpu_registers.SP;
        sys.cpu_registers.RL = code_end + MAX_STACK_SIZE;
        pthread_create(&dma, NULL, (void*)dma_loop, NULL);
        sys.dma_controller.dma_id = dma;
        pthread_create(&cpu, NULL, (void*)mainloop, NULL);
        pthread_join(cpu, NULL);
        init();
    };
};

//Menu Funcional
void menu(){
    int op = 0;
    while(op != 4){
        printf("---Menu---\n1)Iniciar en Modo Normal\n2)Iniciar en modo Debug\n3)Limpiar Log\n4)Apagar\n");
        if (scanf("%d", &op) != 1) {
            // Limpiar buffer o salir en EOF
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            if (c == EOF) break;
            printf("Entrada invalida\n");
            continue;
        }
        switch (op){
        case 1:
            sys.debug_mode_enabled = 0;
            startProgram();
        break;
        case 2:
            sys.debug_mode_enabled = 1;
            startProgram();
        break;
        case 3:
            clean_log();
            printf("log Eliminado...\n");
        break;
        case 4:
            printf("Cerrrando...\n");
        break;
        default:
            printf("Opcion Invalida\n");
        break;
        }
    };

};
int main(){
    init();
    init_disk();
    //Definimos el tiempo inical entre interrupciones
    sys.time_interruption = 12;
    menu();
    return 0;
};
