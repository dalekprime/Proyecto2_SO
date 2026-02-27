#include "debugger.h"

void debug(){
    printf("\nDEBUG >> Instruccion Actual MAR: %d | IR: %d| AC: %d\n",
        sys.cpu_registers.MAR,
        sys.cpu_registers.IR,
        sys.cpu_registers.AC);
    int op = 0;
    while(op != 10){
        printf("\nDebug >> Registros:\n1)AC 2)MAR 3)MDR 4)IR\n5)RB 6)RL 7)RX 8)SP 9)PC\n10)Continuar\n");
        scanf("%d", &op);
        switch(op){
            case 1:
                printf("Debug >> AC: %d\n", sys.cpu_registers.AC);
            break;
            case 2:
                printf("Debug >> MAR: %d\n", sys.cpu_registers.MAR);
            break;
            case 3:
                printf("Debug >> MDR: %d\n", sys.cpu_registers.MDR);
            break;
            case 4:
                printf("Debug >> IR: %d\n", sys.cpu_registers.IR);
            break;
            case 5:
                printf("Debug >> RB: %d\n", sys.cpu_registers.RB);
            break;
            case 6:
                printf("Debug >> RL: %d\n", sys.cpu_registers.RL);
            break;
            case 7:
                printf("Debug >> RX: %d\n", sys.cpu_registers.RX);
            break;
            case 8:
                printf("Debug >> SP: %d\n", sys.cpu_registers.SP);
            break;
            case 9:
                printf("Debug >> PC: %d\n", sys.cpu_registers.PSW.pc);
            break;
            case 10:
                printf("Presione Enter para continuar...\n");
            break;
            default:
                printf("Opcion Invalida\n");
            break;
        };
    };
};