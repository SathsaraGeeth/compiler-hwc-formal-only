/*
 * cache_pkg.sv
 *
 * 2026
 */

package cache_pkg;

import core_pkg::*;
import coherency_pkg::*;


// id tracking
parameter int CPU_REQ_ID_W  = 8;
parameter int TRANS_ID_W    = 8;


// cache geometry
parameter int NUM_WAYS      = 4;
parameter int NUM_SETS      = 64;

parameter int BYTE_OFFSET_W = $clog2(CACHE_LINE_W/8);
parameter int SET_INDEX_W   = $clog2(NUM_SETS);
parameter int WAY_INDEX_W   = (NUM_WAYS > 1) ? $clog2(NUM_WAYS) : 1;
parameter int TAG_W         = PHY_ADDR_W - BYTE_OFFSET_W - SET_INDEX_W;

typedef struct packed {
    logic                  valid;
    logic [TAG_W-1:0]      tag;
    line_state_e           state;
    logic [1:0]            rrpv;
} cache_meta_t;

typedef struct packed {
    logic [TAG_W-1:0]         tag;
    logic [SET_INDEX_W-1:0]   index;
    logic [BYTE_OFFSET_W-1:0] offset;
} cache_addr_t;

function automatic cache_addr_t decode_cache_addr(input logic [PHY_ADDR_W-1:0] addr);
    cache_addr_t decoded_addr;
    decoded_addr.tag    = addr[PHY_ADDR_W-1 -: TAG_W];
    decoded_addr.index  = addr[BYTE_OFFSET_W +: SET_INDEX_W];
    decoded_addr.offset = addr[0 +: BYTE_OFFSET_W];
    return decoded_addr;
endfunction: decode_cache_addr

function automatic logic cache_line_is_valid(input line_state_e state);
    return (state == LINE_M) || (state == LINE_E) || (state == LINE_S);
endfunction: cache_line_is_valid

function automatic logic cache_line_is_dirty(input line_state_e state);
    return state == LINE_M;
endfunction: cache_line_is_dirty

/*
 * Read miss:     I -> IS -> S/E
 * Write miss:    I -> IM -> M
 * Write upgrade: S -> SM -> M
 */
function automatic line_state_e get_acquire_line_state(
    input line_state_e state,
    input cpu_mem_op_e op
);
    if (!cpu_op_is_write(op)) begin
        return LINE_IS;
    end else if (state == LINE_S) begin
        return LINE_SM;
    end else begin
        return LINE_IM;
    end
endfunction: get_acquire_line_state

function automatic logic [PHY_ADDR_W-1:0] get_cache_line_addr(
    input logic [PHY_ADDR_W-1:0] addr
);
    return {addr[PHY_ADDR_W-1:BYTE_OFFSET_W], {BYTE_OFFSET_W{1'b0}}};
endfunction: get_cache_line_addr



// sram data storage
parameter int DATA_SRAM_DEPTH  = NUM_SETS * NUM_WAYS;
parameter int DATA_SRAM_ADDR_W = $clog2(DATA_SRAM_DEPTH);

function automatic logic [DATA_SRAM_ADDR_W-1:0] get_data_sram_addr(
    input logic [SET_INDEX_W-1:0] set_index,
    input logic [WAY_INDEX_W-1:0] way
);
    return (set_index * NUM_WAYS) + way;
endfunction: get_data_sram_addr



// atomics
function automatic logic [XLEN-1:0] execute_atomic(
    input cpu_mem_op_e      op,
    input logic [XLEN-1:0]  old_value,
    input logic [XLEN-1:0]  operand,
    input logic [XLEN-1:0]  compare_value
);
    case (op)
        CPU_AMO_XCHG:    return operand;
        CPU_AMO_ADD:     return old_value + operand;
        CPU_AMO_SUB:     return old_value - operand;
        CPU_AMO_AND:     return old_value & operand;
        CPU_AMO_OR:      return old_value | operand;
        CPU_AMO_XOR:     return old_value ^ operand;
        CPU_AMO_NOT:     return ~old_value;
        CPU_AMO_NEG:     return -old_value;
        CPU_AMO_CMPXCHG: return (old_value == compare_value) ? operand : old_value;
        default:         return old_value;
    endcase
endfunction: execute_atomic




// mshr
parameter int NUM_MSHR_ENTRY = 1 << TRANS_ID_W;

typedef enum logic [2:0] {
    MSHR_FREE,
    MSHR_EVICT,
    MSHR_ACQUIRE,
    MSHR_WAIT,
    MSHR_MEMORY_WAIT
} mshr_state_e;

typedef struct packed {
    mshr_state_e                     state;
    logic        [CPU_REQ_ID_W-1:0]  cpu_id;

    cpu_mem_op_e                     op;
    logic        [PHY_ADDR_W-1:0]    addr;
    logic        [XLEN-1:0]          wdata;
    logic        [XLEN/8-1:0]        wstrb;
    logic        [XLEN-1:0]          compare_data;
    
    logic        [WAY_INDEX_W-1:0]   way;
    logic        [CACHE_LINE_W-1:0]  line_data;
    
    logic                            victim_dirty;
    logic        [PHY_ADDR_W-1:0]    victim_addr;
    logic        [CACHE_LINE_W-1:0]  victim_data;
    line_state_e                     grant_state;
    logic                            grant_received;
    logic                            data_received;
} mshr_entry_t;

function automatic logic [CACHE_LINE_W-1:0] merge_cache_line(
    input logic [CACHE_LINE_W-1:0]  line_data,
    input logic [BYTE_OFFSET_W-1:0] byte_offset,
    input logic [XLEN-1:0]          write_data,
    input logic [XLEN/8-1:0]        write_strobe
);
    logic [CACHE_LINE_W-1:0] merged_data;

    merged_data = line_data;
    for (int i = 0; i < XLEN/8; i++) begin
        if (write_strobe[i]) begin
            merged_data[(byte_offset+i)*8 +: 8] = write_data[i*8 +: 8];
        end
    end
    return merged_data;
endfunction: merge_cache_line


endpackage: cache_pkg
