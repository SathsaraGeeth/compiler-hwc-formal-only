/*
 * interface_pkg.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. SV interfaces are intentionally avoided to
 *    avoid compatibility issues with different tools
 * 2. Purpose of each interface:
 *    a. cpu_req_t: access to memory system from CPU
 *    b. cpu_rsp_t: response to CPU from memory system
 *    c. coh_req_t: coherence request from cache to directory
 *    d. coh_rsp_t: coherence permission response from directory to cache
 *    e. coh_snp_t: snoop command from directory to cache
 *    f. coh_ack_t: snoop acknowledgement from cache to directory
 *    g. cache_fwd_t: cache line data forwarded between caches
 *    h. mem_req_t: cache line read/write request from cache to main memory
 *    i. mem_rsp_t: cache line read data from main memory to cache
 * 3. Stores are treated as fire and forget from the cpu perspective
 *    (Cache subsystem takes the responsibility)
 */

package interface_pkg;

import core_pkg::*;
import cache_pkg::*;
import coherency_pkg::*;

parameter int MEM_REQ_ID_W = $clog2(NUM_CACHES + 2);

/* cpu <-> cache */
typedef struct packed {
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CPU_REQ_ID_W-1:0]    id;
    cpu_mem_op_e                op;
    mem_type_e                  mem_type;
    logic [2:0]                 size;         // in Bytes
    logic [XLEN-1:0]            wdata;
    logic [XLEN-1:0]            compare_data; // for AMO_CMPXCHG
    logic [XLEN/8-1:0]          wstrb;
} cpu_req_t;

typedef struct packed {
    logic [CPU_REQ_ID_W-1:0]    id;
    logic                       atomic_success;
    logic [XLEN-1:0]            rdata;
} cpu_rsp_t;

/* cache <-> directory */
typedef struct packed {
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CACHE_ID_W-1:0]      src_id;
    logic [TRANS_ID_W-1:0]      trans_id;
    coh_req_e                   msg;
} coh_req_t;

typedef struct packed {
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CACHE_ID_W-1:0]      dst_id;
    logic [TRANS_ID_W-1:0]      trans_id;
    logic                       memory_required;
    coh_rsp_e                   msg;
} coh_rsp_t;

typedef struct packed {
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CACHE_ID_W-1:0]      requester_id; // orginal requester
    logic [CACHE_ID_W-1:0]      dst_id;       // cache that needs to snoop
    logic [TRANS_ID_W-1:0]      trans_id;     // original transaction
    coh_snp_e                   msg;
} coh_snp_t;

typedef struct packed {
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CACHE_ID_W-1:0]      src_id;
    logic [CACHE_ID_W-1:0]      requester_id;
    logic [TRANS_ID_W-1:0]      trans_id;
    coh_ack_e                   msg;
} coh_ack_t;

/* cache <-> cache */
typedef struct packed {
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CACHE_ID_W-1:0]      src_id;
    logic [CACHE_ID_W-1:0]      dst_id;
    logic [CACHE_LINE_W-1:0]    data;
    logic [CACHE_LINE_W/8-1:0]  wstrb;
    logic [TRANS_ID_W-1:0]      trans_id;
} cache_fwd_t;

/* cache <-> memory */
typedef struct packed {
    logic                       wen;
    logic [MEM_REQ_ID_W-1:0]    requester_id;
    logic [TRANS_ID_W-1:0]      trans_id;
    logic [PHY_ADDR_W-1:0]      addr;
    logic [CACHE_LINE_W-1:0]    wdata;
    logic [CACHE_LINE_W/8-1:0]  wstrb;
} mem_req_t;

typedef struct packed {
    logic [MEM_REQ_ID_W-1:0]    requester_id;
    logic [TRANS_ID_W-1:0]      trans_id;
    logic [CACHE_LINE_W-1:0]    rdata;
} mem_rsp_t;

endpackage: interface_pkg
