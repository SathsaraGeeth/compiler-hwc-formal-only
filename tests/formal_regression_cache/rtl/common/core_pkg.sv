/*
 * core_pkg.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. 
 */

package core_pkg;

parameter int XLEN          = 64;
parameter int PHY_ADDR_W    = 64;
parameter int MEM_SIZE      = 1024;

typedef enum logic [3:0] {
    CPU_LOAD,
    CPU_STORE,
    CPU_READ_EXCLUSIVE,
    CPU_LR,
    CPU_SC,
    CPU_AMO_XCHG,
    CPU_AMO_ADD,
    CPU_AMO_SUB,
    CPU_AMO_AND,
    CPU_AMO_OR,
    CPU_AMO_XOR,
    CPU_AMO_NOT,
    CPU_AMO_NEG,
    CPU_AMO_CMPXCHG
} cpu_mem_op_e;

typedef enum logic {
    MEM_WB,
    MEM_UC
} mem_type_e;

function automatic logic cpu_op_is_atomic(input cpu_mem_op_e op);
    return op inside {
        CPU_AMO_XCHG, CPU_AMO_ADD, CPU_AMO_SUB, CPU_AMO_AND,
        CPU_AMO_OR, CPU_AMO_XOR, CPU_AMO_NOT, CPU_AMO_NEG,
        CPU_AMO_CMPXCHG
    };
endfunction: cpu_op_is_atomic

function automatic logic cpu_op_is_write(input cpu_mem_op_e op);
    return (op == CPU_STORE) || (op == CPU_SC) || cpu_op_is_atomic(op);
endfunction: cpu_op_is_write

function automatic logic cpu_op_is_exclusive(input cpu_mem_op_e op);
    return op inside {CPU_READ_EXCLUSIVE, CPU_LR, CPU_SC} ||
           cpu_op_is_atomic(op);
endfunction: cpu_op_is_exclusive

function automatic logic cpu_op_is_locked(input cpu_mem_op_e op);
    return cpu_op_is_atomic(op) || op inside {CPU_LR, CPU_SC};
endfunction: cpu_op_is_locked

endpackage: core_pkg
