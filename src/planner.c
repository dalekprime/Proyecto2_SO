#include "planner.h"

void short_planner(int new_state){
    //SALVAGUARDAR PROCESO ACTUAL
    if (sys.current_pid != -1) {
        //Extraemos el PC real
        int real_pc = memory_read(sys.cpu_registers.SP);
        sys.cpu_registers.SP--;
        //Guardamos los Registros
        CPU_REGISTERS temp = sys.cpu_registers;
        temp.PSW.pc = real_pc;
        temp.PSW.operation_mode = 0; 
        temp.PSW.interruptions_enabled = 1;
        sys.process_table[sys.current_pid].data = temp;
        if(new_state == 1){
            //Colocamos el Proceso en la Cola de Listos
            sys.process_table[sys.current_pid].state = READY;
            sys.ready_queue[sys.ready_tail] = sys.current_pid;
            sys.ready_tail = (sys.ready_tail + 1) % MULTIPROGRAMING_GRADE;
        }
        else if(new_state == 3){
            //Colocamos el Proceso en la Cola de Wait
            sys.process_table[sys.current_pid].state = WAITING;
            sys.waiting_queue[sys.waiting_tail] = sys.current_pid;
            sys.waiting_tail = (sys.waiting_tail + 1) % MULTIPROGRAMING_GRADE;
        }
        char log_msg[256];
        sprintf(log_msg, "KERNEL >> Quantum Agotado. Saliente: PID %d", sys.current_pid);
        write_in_log(log_msg);
    }
    //REVISAR PROCESOS DORMIDOS
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
        sys.cpu_registers.PSW.operation_mode = 1;
        sys.cpu_registers.PSW.pc = 0;
    }
}