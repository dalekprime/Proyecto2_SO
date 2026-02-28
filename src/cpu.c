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
    while(1){
        if (sys.dma_controller.shutdown) {
            //Esperamos la Ultima Instruccion del DMA
            while (sys.dma_controller.active || sys.pending_interrupt != INT_NONE) {
                check_interruptions();
            }
            break;
        }
        if(sys.current_pid == -1 && sys.ready_head == sys.ready_tail){
            if (sys.active_process == 0) {
                //No hay procesos, la cpu duerme
                sem_wait(&sys.cpu_wakeup);
                sys.cpu_registers.PSW.interruptions_enabled = 1;
                internal_timer = 0;
                sys.pending_interrupt = INT_TIMER;
                check_interruptions();
                continue;
            } else {
                //Avanza el Reloj, Si hay procesos dormidos
                sys.time += 1;
                internal_timer += 1;
                if(internal_timer >= sys.time_interruption){
                    sys.cpu_registers.PSW.interruptions_enabled = 1;
                    sys.pending_interrupt = INT_TIMER;
                    internal_timer = 0;
                }
                check_interruptions();
                continue;
            }
        }
        //Fetch Phase
        sys.cpu_registers.MAR =  sys.cpu_registers.PSW.pc;
        if(sys.cpu_registers.PSW.operation_mode == 0){
            sys.cpu_registers.MAR += sys.cpu_registers.RB;
        };
        sys.cpu_registers.MDR = memory_read(sys.cpu_registers.MAR);
        sys.cpu_registers.IR = sys.cpu_registers.MDR;
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
                    int param;
                    char log_msg[256];
                    switch (sys.cpu_registers.AC){
                        case 1:
                            //Terminar Programa
                            param = memory_read(sys.cpu_registers.SP);
                            sys.cpu_registers.SP--;
                            sprintf(log_msg, "\n>> Proceso %d finalizado con estado: %d\n", sys.current_pid, param);
                            write_in_log(log_msg);
                            printf("\n>> Proceso %d finalizado con estado: %d\n", sys.current_pid, param);
                            //Libera Memoria
                            int block_index = (sys.cpu_registers.RB - OS_MEM_RESERVED) / MEMORY_BLOCK_SIZE;
                            sys.memory_blocks[block_index] = false;
                            //Marcar como terminado
                            sys.process_table[sys.current_pid].state = TERMINATED;
                            sys.active_process--;
                            //Forzar al Planificador a meter el siguiente proceso
                            sys.pending_interrupt = INT_TIMER;
                        break;
                        case 2:
                            //Imprimir por Pantalla
                            param = memory_read(sys.cpu_registers.SP);
                            sys.cpu_registers.SP--;
                            sprintf(log_msg, "\n[Proceso %d] >> %d\n", sys.current_pid, param);
                            write_in_log(log_msg);
                            printf("\n[Proceso %d] >> %d\n", sys.current_pid, param);
                            //Devolver control al Usuario
                            sys.cpu_registers.PSW.pc = 99;
                        break;
                        case 3:
                            //Leer por Pantalla
                            sprintf(log_msg, "\n[Proceso %d] >> Lectura por Pantalla\n", sys.current_pid);
                            write_in_log(log_msg);
                            printf("\n[Proceso %d] Ingrese un valor entero: ", sys.current_pid);
                            scanf("%d", &param);
                            sys.cpu_registers.AC = param;
                            //Devolver control al Usuario
                            sys.cpu_registers.PSW.pc = 99;
                        break;
                        case 4:
                            //Dormir
                            param = memory_read(sys.cpu_registers.SP);
                            sys.cpu_registers.SP--;
                            //SALVAGUARDAR ESTADO
                            int real_pc = memory_read(sys.cpu_registers.SP);
                            sys.cpu_registers.SP--;
                            sys.cpu_registers.PSW.pc = real_pc;
                            sys.cpu_registers.PSW.operation_mode = 0; 
                            sys.cpu_registers.PSW.interruptions_enabled = 1;
                            sys.process_table[sys.current_pid].data = sys.cpu_registers;
                            //Pasamos a bloqueado
                            sys.process_table[sys.current_pid].state = WAITING;
                            sys.process_table[sys.current_pid].wake_up_time = sys.time + param;
                            //Encolar en Bloqueados
                            sys.waiting_queue[sys.waiting_tail] = sys.current_pid;
                            sys.waiting_tail = (sys.waiting_tail + 1) % MULTIPROGRAMING_GRADE;
                            sys.pending_interrupt = INT_TIMER;
                        break;
                        default:
                            //Llamada Invalida
                            sys.pending_interrupt = INT_SYSCALL_INVALID;
                        break;
                    }
                }else{
                    continue;
                };
            break;
            //INT_TIMER
            case 93:
                if(sys.cpu_registers.PSW.operation_mode == 1){
                    write_in_log("KERNEL >> Interrupcion de Reloj");
                    //SALVAGUARDAR PROCESO ACTUAL
                    if (sys.current_pid != -1 && sys.process_table[sys.current_pid].state == RUNNING) {
                        //Extraemos el PC real
                        int real_pc = memory_read(sys.cpu_registers.SP);
                        sys.cpu_registers.SP--;
                        sys.cpu_registers.PSW.pc = real_pc;
                        //Incluimos el Proceso en la Cola de LISTOS
                        sys.cpu_registers.PSW.operation_mode = 0; 
                        sys.cpu_registers.PSW.interruptions_enabled = 1;
                        //Guardamos los Registros
                        sys.process_table[sys.current_pid].data = sys.cpu_registers;
                        sys.process_table[sys.current_pid].state = READY;
                        sys.ready_queue[sys.ready_tail] = sys.current_pid;
                        sys.ready_tail = (sys.ready_tail + 1) % MULTIPROGRAMING_GRADE;
                        char log_msg[256];
                        sprintf(log_msg, "KERNEL >> Quantum Agotado. Saliente: PID %d", sys.current_pid);
                        write_in_log(log_msg);
                    }
                    internal_timer = 0;
                    //Revisar Procesos Dormidos
                    for (int i = 0; i < MULTIPROGRAMING_GRADE; i++) {
                        if (sys.process_table[i].state == WAITING) {
                            if (sys.time >= sys.process_table[i].wake_up_time) {
                                //Despertar
                                sys.process_table[i].state = READY;
                                sys.ready_queue[sys.ready_tail] = i;
                                sys.ready_tail = (sys.ready_tail + 1) % MULTIPROGRAMING_GRADE;
                                char log_msg[256];
                                sprintf(log_msg, "KERNEL >> Proceso %d despertado y movido a LISTO", i);
                                write_in_log(log_msg);
                            }
                        }
                    }
                    //CARGAR SIGUIENTE PROCESO
                    if (sys.ready_head != sys.ready_tail) {
                        sys.current_pid = sys.ready_queue[sys.ready_head];
                        sys.ready_head = (sys.ready_head + 1) % MULTIPROGRAMING_GRADE;
                        sys.process_table[sys.current_pid].state = RUNNING;
                        sys.cpu_registers = sys.process_table[sys.current_pid].data;
                        char log_msg[256];
                        sprintf(log_msg, "KERNEL >> Cambio de Contexto. Entrante: PID %d", sys.current_pid);
                        write_in_log(log_msg);
                    } else {
                        //Si la cola está vacía, la CPU entra en Reposo
                        sys.current_pid = -1;
                    }
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
                if (sys.cpu_registers.PSW.operation_mode == 0) {
                    char log_msg[256];
                    sprintf(log_msg, "KERNEL >> Proceso %d llego a instruccion HALT. Terminando.", sys.current_pid);
                    write_in_log(log_msg);
                    //Libera la memoria
                    int block_index = (sys.cpu_registers.RB - OS_MEM_RESERVED) / MEMORY_BLOCK_SIZE;
                    sys.memory_blocks[block_index] = false;
                    //Actualiza el BCP
                    sys.process_table[sys.current_pid].state = TERMINATED;
                    sys.active_process--;
                    //Llama al planificador para que asigne el siguiente trabajo
                    sys.pending_interrupt = INT_TIMER; 
                } else {
                    //Si lo llama el SO
                    while (sys.dma_controller.active){ usleep(100); };
                    sys.dma_controller.shutdown = true;
                }
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
        //Timer
        if (sys.cpu_registers.PSW.operation_mode == 0) {
            sys.time += 1;
            if (sys.pending_interrupt == INT_NONE) {
                sys.cpu_registers.PSW.pc += 1;
                internal_timer += 1;
                if (sys.time_interruption > 0 && internal_timer >= sys.time_interruption) {
                    sys.pending_interrupt = INT_TIMER;
                }
            }
        }
        if (sys.debug_mode_enabled == 1) debug();
        check_interruptions();
    };
    return NULL;
};

