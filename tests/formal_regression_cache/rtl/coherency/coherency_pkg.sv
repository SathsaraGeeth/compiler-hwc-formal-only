/*
 * coherency_pkg.sv
 * 
 * 2026
 */

/*
 * Comments:
 * 1. This file owns the definition of the coherency protocol
 * 2. The protocol is MOESDIF [1] (near the EOF) from it we have implemented the subset MESI
 * 3. The state transtion rules [2] (near the EOF)
 * 4. Cachable locks and Bus locks [3] (ref a, pages: 196, 197 translated to RISC semantics)
 *   a. cachable lock: aligned AMO, LR/SC on cachable memory
 *   b. uncachable lock: unaligned AMO, LR/SC or AMO on uncachable memory (e.g. MMIO)
 * 5. Reference(s):
 *   a. https://docs.amd.com/v/u/en-US/24593_3.45_APM_Vol2 (pages: 193-197)
 */

package coherency_pkg;

import core_pkg::*;

parameter int NUM_CACHES      = 2;
parameter int NUM_DIR_ENTRIES = MEM_SIZE;
parameter int CACHE_LINE_W    = 512;
parameter int CACHE_ID_W      = (NUM_CACHES > 1) 
                              ? $clog2(NUM_CACHES) : 1;

/*
 * Read [1]
 */
typedef enum logic [3:0] {
    LINE_I,         // invalid
    LINE_E,         // exclusive
    LINE_S,         // shared
    // LINE_F,         // forward
    LINE_M,         // modified
    // LINE_D,         // dirty
    // LINE_O,         // owned

    LINE_IS,        // invalid to shared
    LINE_IM,        // invalid to modified
    LINE_SM         // shared to modified
} line_state_e;

/*
 * Read [2]
 */
typedef enum logic [3:0] { 
    LINE_READ,
    LINE_WRITE,
    LINE_PROBE_FOR_READ,
    LINE_PROBE_FOR_WRITE,
    LINE_READ_RO_CLEAN,
    // LINE_READ_RO_WRITTEN,
    LINE_READ_WRITABLE_CLEAN,
    // LINE_READ_WRITABLE_WRITTEN,
    LINE_RESET,
    LINE_INVD,
    LINE_WB_INVD
} line_trans_e;



/*
 * Related to uncachable locks
 * XLEN = 64b
 * LINE = 512b
 * So an misaligned AMO can span 2 lines
 * SNOOP and invalidate the lines
 */
typedef enum logic [3:0]{
    BUS_LOCK_IDLE,
    BUS_LOCK_SNOOP_REQ_LO,
    BUS_LOCK_SNOOP_WAIT_LO,
    BUS_LOCK_SNOOP_REQ_HI,
    BUS_LOCK_SNOOP_WAIT_HI,
    BUS_LOCK_READ_REQ_LO,
    BUS_LOCK_READ_RESP_LO,
    BUS_LOCK_READ_REQ_HI,
    BUS_LOCK_READ_RESP_HI,
    BUS_LOCK_WRITE_LO,
    BUS_LOCK_WRITE_HI,
    BUS_LOCK_RESP
} bus_lock_state_e;

typedef enum logic [1:0] {
    BUS_LOCK_MISALIGNED_CACHEABLE,
    BUS_LOCK_MISALIGNED_UNCACHEABLE,
    BUS_LOCK_ALIGNED_UNCACHEABLE,
    BUS_LOCK_ILLEGAL
} bus_lock_type_e;

function automatic logic is_bus_lock(
    input cpu_mem_op_e           op,
    input mem_type_e             mem_type,
    input logic [PHY_ADDR_W-1:0] addr,
    input logic [2:0]            size
);
    logic [PHY_ADDR_W:0]   access_bytes;
    logic [PHY_ADDR_W-1:0] alignment_mask;

    access_bytes   = (PHY_ADDR_W+1)'(1) << size;
    alignment_mask = PHY_ADDR_W'(access_bytes - 1'b1);

    return cpu_op_is_atomic(op)  &&
           ((mem_type != MEM_WB) ||
           ((addr & alignment_mask) != '0) ||
           (((addr % (CACHE_LINE_W/8)) + access_bytes) >
           (CACHE_LINE_W/8)));
endfunction: is_bus_lock

function automatic bus_lock_type_e get_bus_lock_type(
    input cpu_mem_op_e           op,
    input mem_type_e             mem_type,
    input logic [PHY_ADDR_W-1:0] addr,
    input logic [2:0]            size
);
    logic [PHY_ADDR_W-1:0] alignment_mask;

    alignment_mask = (PHY_ADDR_W'(1) << size) - 1'b1;

    if (!is_bus_lock(op, mem_type, addr, size)) begin
        return BUS_LOCK_ILLEGAL;
    end else if (mem_type == MEM_WB) begin
        return BUS_LOCK_MISALIGNED_CACHEABLE;
    end else if ((addr & alignment_mask) != '0) begin
        return BUS_LOCK_MISALIGNED_UNCACHEABLE;
    end else begin
        return BUS_LOCK_ALIGNED_UNCACHEABLE;
    end
endfunction: get_bus_lock_type



/*
 * Read [2]
 */
function automatic line_state_e get_next_line_state(
    input line_state_e state,
    input line_trans_e trans
);
    if ((trans == LINE_RESET)    || 
        (trans == LINE_INVD)     ||
        (trans == LINE_WB_INVD)) begin
    
        return LINE_I;
    end

    case (state)
        LINE_I: case (trans)
            LINE_READ_RO_CLEAN:         return LINE_S;
            LINE_READ_WRITABLE_CLEAN:   return LINE_E;
            LINE_WRITE:                 return LINE_M;
            default:                    return LINE_I;
        endcase
        LINE_E: case (trans)
            LINE_READ:                  return LINE_E;
            LINE_WRITE:                 return LINE_M;
            LINE_PROBE_FOR_READ:        return LINE_S;
            LINE_PROBE_FOR_WRITE:       return LINE_I;
            default:                    return LINE_I;
        endcase
        LINE_S: case (trans)
            LINE_READ:                  return LINE_S;
            LINE_PROBE_FOR_READ:        return LINE_S;
            LINE_PROBE_FOR_WRITE:       return LINE_I;
            LINE_WRITE:                 return LINE_M;
            default:                    return LINE_I;
        endcase
        // LINE_F: case (trans)
        //     LINE_READ:                  return LINE_F;
        //     LINE_WRITE:                 return LINE_M;
        //     LINE_PROBE_FOR_READ:        return LINE_S;
        //     default:                    return LINE_I;
        LINE_M: case (trans)
            LINE_READ:                  return LINE_M;
            LINE_WRITE:                 return LINE_M;
            LINE_PROBE_FOR_READ:        return LINE_S;
            LINE_PROBE_FOR_WRITE:       return LINE_I;
            default:                    return LINE_I;
        endcase
        // LINE_D: case (trans)
        //     LINE_READ:                  return LINE_D;
        //     LINE_WRITE:                 return LINE_M;
        //     LINE_PROBE_FOR_READ:        return LINE_S;
        //     default:                    return LINE_I;
        // endcase
        // LINE_O: case (trans)
        //     LINE_READ:                  return LINE_O;
        //     LINE_PROBE_FOR_READ:        return LINE_O;
        //     LINE_WRITE:                 return LINE_M;
        //     default:                    return LINE_I;
        default:                        return LINE_I;
    endcase
endfunction: get_next_line_state

/*
 * cache requests premission or says 
 * or what it being to the directory
 *
 * GETS: load miss
 * GETM: store miss or AMO need writes ownership
 * PUTS: evict clean line from S or E
 * PUTM: evict dirty line from M
 */
typedef enum logic [1:0] {
    GETS,               // request shared/read permission
    GETM,               // request write ownership
    PUTS,               // release clean/shared line
    PUTM                // write back dirty line
} coh_req_e;

/*
 * directory's reponses to the 
 * cache permission requests
 *
 * GRANT_S: other caches has valid shared/readble copy -> join as a sharer
 * GRANT_E: ask for GETS and no other cache has a copy -> grant exclusive clean permission
 * GRANT_M: other copies are invalidiated and grant write ownership
 * RETRY: retry request later
 */
typedef enum logic [2:0] {
    GRANT_S,            // grant shared permission
    // GRANT_F,         // MOESDIF forwarder grant
    // GRANT_O,         // MOESDIF owned grant
    GRANT_E,            // grant exclusive clean permission
    GRANT_M,            // grant modified ownership
    // GRANT_D,         // MOESDIF dirty grant
    RETRY               // retry request later
} coh_rsp_e;

/*
 * directory to existing cache commands
 *
 * SNP_INV: others have a clean copy and this cache need write ownership
 * SNP_FWD_GETS: another cache has a clean copy and reqeuster need to read it
 * SNP_FWD_GETM: another cache has a dirty copy and requester need write ownership
 */
typedef enum logic [1:0] {
    SNP_INV,            // invalidate cache line
    SNP_FWD_GETS,       // forward data for read request
    SNP_FWD_GETM        // forward data and invalidate copy
} coh_snp_e;

/*
 * cache acknolwedgement to directory snoop commands
 *
 * ACK_INV: had the line and invalidated it
 * ACK_DATA: forwarded the data successfully
 * ACK_NOT_PRESENT: snooped but the cache dont have that line
 */
typedef enum logic [2:0] {
    ACK_INV,            // invalidation completed
    ACK_DATA,           // data forwarded successfully
    // ACK_DIRTY_KEEP,     // MOESDIF owner keeps dirty data
    // ACK_DIRTY_TRANSFER, // MOESDIF dirty ownership transfer
    ACK_NOT_PRESENT     // cache line not present
} coh_ack_e;

endpackage: coherency_pkg



/*
 * [1]
 * Copied from the AMD APM Vol2 reference (page: 193) for reference
 *
 * Cache Coherence States (MOESDIF)
 *
 * Invalid (I):
 *   - Cache line does not contain a valid copy of the data.
 *   - A valid copy may exist in main memory or another processor cache.
 *
 * Exclusive (E):
 *   - Cache line contains the most recent and correct copy of the data.
 *   - Main memory also contains the most recent and correct copy.
 *   - No other processor holds a copy of the data.
 *
 * Shared (S):
 *   - Cache line contains the most recent and correct copy of the data.
 *   - Other processors may also hold copies in the Shared state.
 *   - One other processor may hold the line in Owned or Forward state.
 *   - If no processor owns the line in Owned state, main memory also
 *     contains the most recent copy.
 *
 * Forward (F):
 *   - Cache line contains the most recent and correct copy of the data.
 *   - Other processors may hold copies in the Shared state.
 *   - Acts as a performance hint indicating which cache should respond
 *     with data when a broadcast probe occurs.
 *
 * Modified (M):
 *   - Cache line contains the most recent and correct copy of the data.
 *   - Main memory contains a stale (incorrect) copy.
 *   - No other processor holds a copy.
 *   - Data has usually been modified after being loaded into the cache.
 *
 * Dirty (D):
 *   - Cache line contains the most recent and correct copy of the data.
 *   - Main memory contains a stale (incorrect) copy.
 *   - No other processor holds a copy.
 *   - Data has usually not been modified after being loaded into the cache.
 *
 * Owned (O):
 *   - Cache line contains the most recent and correct copy of the data.
 *   - Other processors may also hold copies of the data.
 *   - Unlike Shared state, main memory may contain a stale copy.
 *   - Only one processor may hold the line in the Owned state.
 *   - Other processors holding the line must be in the Shared state.
 */


/*
 * [2]
 * Based on Figure 7.2 in page 194 and text in page 195
 *
 * MOESDIF State Transition Rules (minus the concerns like virtual memory complexity)
 *
 *   READ:
 *       Obtain latest copy from memory or another cache
 *       READ_RO_CLEAN:
 *           I -> F
 *       READ_RO_WRITTEN:
 *           I -> O
 *   WRITE:
 *       Request writable ownership
 *       READ_WRITABLE_CLEAN:
 *           I -> E
 *       READ_WRITABLE_WRITTEN:
 *           I -> D
 *       WRITE:
 *           I -> M
 *   READ:
 *       E -> E
 *   WRITE:
 *       Data becomes dirty
 *       E -> M
 *   PROBE_FOR_READ:
 *       Another cache requests a read copy
 *       E -> S
 *   PROBE_FOR_WRITE:
 *       Another cache requests ownership
 *       E -> I
 *   READ:
 *       S -> S
 *   PROBE_FOR_READ:
 *       Another reader joins
 *       S -> S/F
 *   WRITE:
 *       Request exclusive ownership
 *       Invalidate other sharers
 *       S -> M
 *   PROBE_FOR_WRITE:
 *       Another cache obtains ownership
 *       S -> I
 *   READ:
 *       F -> F
 *   PROBE_FOR_READ:
 *       Transfer forwarder responsibility to the requester
 *       F -> S
 *   WRITE:
 *       F -> M
 *   PROBE_FOR_WRITE:
 *       Ownership requested by another cache
 *       F -> I
 *   READ:
 *       M -> M
 *   WRITE:
 *       M -> M
 *   PROBE_FOR_READ:
 *       Supply dirty data to requester
 *       Allow sharing
 *       M -> O
 *   PROBE_FOR_WRITE:
 *       Supply data and invalidate
 *       M -> I
 *   READ:
 *       O -> O
 *   PROBE_FOR_READ:
 *       Supply data while retaining ownership
 *       O -> O
 *   PROBE_FOR_WRITE:
 *       Supply data and release ownership
 *       O -> I
 *   READ:
 *       D -> D
 *   WRITE:
 *       D -> M
 *   PROBE_FOR_READ:
 *       Transfer dirty ownership to the requester
 *       D -> S
 *   PROBE_FOR_WRITE:
 *       Supply data and invalidate
 *       D -> I
 *   RESET:
 *       Any state -> I
 *   INVD:
 *       Clean states:
 *           E/S/F -> I
 *       Dirty states:
 *           M/D/O -> I
 *           (requires writeback if memory is stale)
 *   WB_INVD:
 *       Write back latest data to memory
 *       M/D/O -> I
 */

/*
 * [3]
 * Bus locks
 * Handles uncacheable locks as blocking memory transactions
 *    a) read the first memory line
 *    b) read the second memory line if the access crosses a boundary
 *       - misaligned ones need, but uncacheable and aligned ones do not
 *    c) execute the atomic operation locally
 *    d) merge and write the affected memory line(s)
 *    e) return the original value to the CPU
 * Normal AMOs don't visit here
 */
