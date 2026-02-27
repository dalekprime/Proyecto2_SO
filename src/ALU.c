#include "ALU.h"

int ALU(int val1, int val2, int op){
    long long res = 0;
    switch (op){
        case SUM:
            res = (long long)val1 + val2;
        break;
        case RES:
            res = (long long)val1 - val2;
        break;
        case MULT:
            res = (long long)val1 * val2;
        break;
        case DIVI:
            if(val2 == 0){
                sys.pending_interrupt = INT_INVALID_INSTR;
                return 0;
            };
            res = (long long)val1 / val2;
        break;
    };
    //Manejo de OverFLow o Underflow
    if(res > MAGNITUDE_LIMIT){
        sys.cpu_registers.PSW.condition_code = 3;
        sys.pending_interrupt = INT_OVERFLOW;
        return MAGNITUDE_LIMIT;
    }else if(res < -1 * MAGNITUDE_LIMIT){
        sys.cpu_registers.PSW.condition_code = 3;
        sys.pending_interrupt = INT_UNDERFLOW;
        return -MAGNITUDE_LIMIT;
    };
    //Actualizacion de Flags
    if(res == 0){
        sys.cpu_registers.PSW.condition_code = 0;
    }else if(res < 0){
        sys.cpu_registers.PSW.condition_code = 1;
    }else if(res > 0){
        sys.cpu_registers.PSW.condition_code = 2;
    };
    return (int)res;
};
