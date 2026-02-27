#include "cpu.h"

//Decodificador de Instrucciones
void decode(int ir, int *opcode, int *addr_mode, int *operand){
    *opcode = ir / 1000000;
    *addr_mode = (ir / 100000) % 10;
    *operand = ir % 100000;
};

//Calcula Operando
int get_operand(int mode, int value){
    switch (mode){
        case 0:
            //Valor en Direccion
            if(sys.cpu_registers.PSW.operation_mode == 0){
                value += sys.cpu_registers.RB;
            };
            return sign_to_int(memory_read(value), 8);
        case 1:
            //Valor Inmediato
            return sign_to_int(value, 5);
        case 2:
            //Valor en Memoria con desplazamiento
            int base = sign_to_int(sys.cpu_registers.AC, 8) + value;
            if(sys.cpu_registers.PSW.operation_mode == 0){
                base += sys.cpu_registers.RB;
            };
            return sign_to_int(memory_read(base), 8);
        default:
            //Codigo invalido
            sys.pending_interrupt = INT_INVALID_INSTR;
            return 0;
    }
};

//Calcula Direcciones
int get_addr(int mode, int value){
    switch (mode){
        case 0:
            //Valor en Direccion
            if(sys.cpu_registers.PSW.operation_mode == 0){
                value += sys.cpu_registers.RB;
            };
            return value;
        case 1:
            //No puede contener ese mode
            sys.pending_interrupt = INT_INVALID_INSTR;
            return -1;
        case 2:
            //Valor en Memoria con desplazamiento
            int eff_addr = sign_to_int(sys.cpu_registers.AC, 8) + value;
            if(sys.cpu_registers.PSW.operation_mode == 0){
                eff_addr += sys.cpu_registers.RB;
            };
            return eff_addr;
        default:
            //Codigo invalido
            sys.pending_interrupt = INT_INVALID_INSTR;
            return 0;
    }
};

//Manejador de Interrupciones
void check_interruptions(){
    if(sys.pending_interrupt != INT_NONE &&
        sys.cpu_registers.PSW.interruptions_enabled){
            //Cambia a modo Kernel
            sys.cpu_registers.PSW.operation_mode = 1;
            sys.cpu_registers.PSW.interruptions_enabled = 0;
            //Salvaguardar Estado
            sys.cpu_registers.SP++;
            memory_write(sys.cpu_registers.SP, sys.cpu_registers.PSW.pc);
            write_in_log("Entrando en modo Kernel...");
            //Logger
            char ins[256];
            sprintf(ins, "Interrupcion Ejecutada: %d", sys.pending_interrupt);
            write_in_log(ins);
            //Manejador de Interrupciones
            sys.cpu_registers.PSW.pc = 100 + sys.pending_interrupt;
            sys.pending_interrupt = INT_NONE;
    };
}

//Main Loop del CPU
void* mainloop(){
    int internal_timer = 0;
    bool end = 0;
    while(1){
        //Interrumpcion de Reloj
        if(internal_timer >= sys.time_interruption){
            sys.pending_interrupt = INT_TIMER;
            internal_timer = 0;
        };
        //Fetch Phase
        sys.cpu_registers.MAR =  sys.cpu_registers.PSW.pc;
        if(sys.cpu_registers.PSW.operation_mode == 0){
            sys.cpu_registers.MAR += sys.cpu_registers.RB;
        };
        sys.cpu_registers.MDR = memory_read(sys.cpu_registers.MAR);
        sys.cpu_registers.IR = sys.cpu_registers.MDR;
        sys.cpu_registers.PSW.pc++;
        //Decode Phase
        int opcode, addr_mode, operand;
        decode(sys.cpu_registers.IR, &opcode, &addr_mode, &operand);
        //Execute Phase
        int val_ac, val_op, res, eff_addr;
        switch (opcode){
            //Sum
            case 0:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = get_operand(addr_mode, operand);
                res = ALU(val_ac, val_op, SUM);
                sys.cpu_registers.AC = int_to_sign(res, 8);
            break;
            //res
            case 1:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = get_operand(addr_mode, operand);
                res = ALU(val_ac, val_op, RES);
                sys.cpu_registers.AC = int_to_sign(res, 8);
            break;
            //mult
            case 2:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = get_operand(addr_mode, operand);
                res = ALU(val_ac, val_op, MULT);
                sys.cpu_registers.AC = int_to_sign(res, 8);
            break;
            //divi
            case 3:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = get_operand(addr_mode, operand);
                res = ALU(val_ac, val_op, DIVI);
                sys.cpu_registers.AC = int_to_sign(res, 8);
            break;
            //load
            case 4:
                val_op = get_operand(addr_mode, operand);
                sys.cpu_registers.AC = int_to_sign(val_op, 8);
            break;
            //str
            case 5:
                eff_addr = get_addr(addr_mode, operand);
                memory_write(eff_addr, sys.cpu_registers.AC);
            break;
            //loadrx
            case 6:
                sys.cpu_registers.AC = sys.cpu_registers.RX;
            break;
            //strrx
            case 7:
                sys.cpu_registers.RX = sys.cpu_registers.AC;
            break;
            //comp
            case 8:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = get_operand(addr_mode, operand);
                ALU(val_ac, val_op, RES);
            break;
            //jmpe
            case 9:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = sign_to_int(memory_read(sys.cpu_registers.SP), 8);
                if(val_ac == val_op){
                    int target = get_addr(addr_mode, operand);
                    if(sys.cpu_registers.PSW.operation_mode == 0){
                        //Para evitar doble Relocalizacion
                        target -= sys.cpu_registers.RB;
                    };
                    sys.cpu_registers.PSW.pc = target;
                };
            break;
            //jmpne
            case 10:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = sign_to_int(memory_read(sys.cpu_registers.SP), 8);
                if(val_ac != val_op){
                    int target = get_addr(addr_mode, operand);
                    if(sys.cpu_registers.PSW.operation_mode == 0){
                        //Para evitar doble Relocalizacion
                        target -= sys.cpu_registers.RB;
                    };
                    sys.cpu_registers.PSW.pc = target;
                };
            break;
            //jmplt
            case 11:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = sign_to_int(memory_read(sys.cpu_registers.SP), 8);
                if(val_ac < val_op){
                    int target = get_addr(addr_mode, operand);
                    if(sys.cpu_registers.PSW.operation_mode == 0){
                        //Para evitar doble Relocalizacion
                        target -= sys.cpu_registers.RB;
                    };
                    sys.cpu_registers.PSW.pc = target;
                };
            break;
            //jmplgt
            case 12:
                val_ac = sign_to_int(sys.cpu_registers.AC, 8);
                val_op = sign_to_int(memory_read(sys.cpu_registers.SP), 8);
                if(val_ac > val_op){
                    int target = get_addr(addr_mode, operand);
                    if(sys.cpu_registers.PSW.operation_mode == 0){
                        //Para evitar doble Relocalizacion
                        target -= sys.cpu_registers.RB;
                    };
                    sys.cpu_registers.PSW.pc = target;
                };
            break;
            //svc
            case 13:
                sys.pending_interrupt = INT_SYSCALL;
                //Salvaguardar Estado
            break;
            //retrn
            case 14:
                //Asumimos que en SP hay una dir de retorno
                sys.cpu_registers.PSW.pc = memory_read(sys.cpu_registers.SP);
            break;
            //hab
            case 15:
                sys.cpu_registers.PSW.interruptions_enabled = 1;
            break;
            //dhab
            case 16:
                sys.cpu_registers.PSW.interruptions_enabled = 0;
            break;
            //tti
            case 17:
                sys.time_interruption = operand;
                internal_timer = 0;
            break;
            //chmod
            case 18:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    sys.cpu_registers.PSW.operation_mode = 0;
                }else{
                    sys.cpu_registers.PSW.operation_mode = 1;
                };
            break;
            //loadrb
            case 19:
                sys.cpu_registers.AC = sys.cpu_registers.RB;
            break;
            //strrb
            case 20:
                sys.cpu_registers.RB = sys.cpu_registers.AC;
            break;
            //loadrl
            case 21:
                sys.cpu_registers.AC = sys.cpu_registers.RL;
            break;
            //strrl
            case 22:
                sys.cpu_registers.RL = sys.cpu_registers.AC;
            break;
            //loadsp
            case 23:
                sys.cpu_registers.AC = sys.cpu_registers.SP;
            break;
            //strsp
            case 24:
                sys.cpu_registers.SP = sys.cpu_registers.AC;
            break;
            //psh
            case 25:
                sys.cpu_registers.SP++;
                memory_write(sys.cpu_registers.SP, sys.cpu_registers.AC);
            break;
            //pop
            case 26:
                sys.cpu_registers.AC = memory_read(sys.cpu_registers.SP);
                sys.cpu_registers.SP--;
            break;
            //j
            case 27:
                int target = get_addr(addr_mode, operand);
                if(sys.cpu_registers.PSW.operation_mode == 0){
                    //Para evitar doble Relocalizacion
                    target -= sys.cpu_registers.RB;
                };
                sys.cpu_registers.PSW.pc = target;
            break;
            //sdmap
            case 28:
                val_op = get_operand(addr_mode, operand);
                sys.dma_controller.selected_track = val_op;
            break;
            //sdmac
            case 29:
                val_op = get_operand(addr_mode, operand);
                sys.dma_controller.selected_cylinder = val_op;
            break;
            //sdmas
            case 30:
                val_op = get_operand(addr_mode, operand);
                sys.dma_controller.selected_sector = val_op;
            break;
            //sdmaio
            case 31:
                val_op = get_operand(addr_mode, operand);
                sys.dma_controller.io_mode = val_op;
            break;
            //sdmam
            case 32:
                val_op = get_operand(addr_mode, operand);
                sys.dma_controller.ram_address = val_op;
            break;
            //sdmaon
            case 33:
                //Iniciar DMA
                sys.dma_controller.active = true;
            break;
            //Devuelve control al Usuario
            case 89:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    //Recargamos para seguir con la ultima instruccion
                    sys.cpu_registers.PSW.pc = memory_read(sys.cpu_registers.SP);
                    sys.cpu_registers.SP--;
                    //Log
                    write_in_log("Volviendo a Modo Usuario...");
                    //Volver a Modo Usuario
                    sys.cpu_registers.PSW.interruptions_enabled = 1;
                    sys.cpu_registers.PSW.operation_mode = 0;
                }else{
                    continue;
                };
            break;
            //INT_SYSCALL_INVALID
            case 90:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Llamada Invalida al Sistema");
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_INVALID_INT
            case 91:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Interrupcion Invalida");
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_SYSCALL
            case 92:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Llamada al Sistema");
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_TIMER
            case 93:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Interrupcion de Reloj");
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_IO_END
            case 94:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    char mes[256];
                    sprintf(mes, "KERNEL >> Operacion I/O Terminada %d", sys.dma_controller.status);
                    write_in_log(mes);
                    sys.dma_controller.active = false;
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_INVALID_INSTR
            case 95:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Instruccion Invalida");
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_INVALID_ADDR
            case 96:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Dirreccion Invalida");
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_UNDERFLOW
            case 97:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Underflow");
                    sys.cpu_registers.AC = -MAGNITUDE_LIMIT;
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //INT_OVERFLOW
            case 98:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Overflow");
                    sys.cpu_registers.AC = MAGNITUDE_LIMIT;
                    //Devolver control al Usuario
                    sys.cpu_registers.PSW.pc = 99;
                }else{
                    continue;
                };
            break;
            //halt
            case 99:
                while (sys.dma_controller.active){};
                sys.dma_controller.shutdown = true;
                check_interruptions();
                pthread_join(sys.dma_controller.dma_id, NULL);
                end = 1;
            break;
            //Instruccion Invalida
            default:
                sys.pending_interrupt = INT_INVALID_INSTR;
            break;
        }
        //Logger
        if(sys.cpu_registers.IR < 89000000 || sys.cpu_registers.IR == 99000000 ){
            char ins[256];
            sprintf(ins, "Instruccion Ejecutada: %d | MAR: %d | AC: %d",
                sys.cpu_registers.IR, sys.cpu_registers.MAR, sys.cpu_registers.AC);
            write_in_log(ins);
        };
        //Debug
        if (sys.debug_mode_enabled == 1) {
            debug();
        }
        sys.time += 1;
        internal_timer += 1;
        check_interruptions();
        //Terminar
        if(end == 1){
            char ins[256];
            sprintf(ins, "Programa terminado en %d...\n", sys.time);
            write_in_log(ins);
            break;
        };
    };
    return NULL;
};

