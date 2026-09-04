/*
 * cache.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. The cache is intentionally kept simple
 *    A. 1 cpu facing rw port + 1 memory facing rw port
 *    B. non blocking cache - uses MSHR
 *    C. rrpv for cache line replacement policy
 *    D. physically indexed, physically tagged for simplicity
 *    E. single SRAM bank for simplicity
 *       no bank interleaving
 *    F. write back cache
 *    G. fences/virtual memory is missing for simplicity
 *    H. still kept enough intresting stuff to non trivial timing hazards
 *    I. MESI subset from MOESDIF + intermediate states
 */

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;


module cache #(
    parameter logic [CACHE_ID_W-1:0] CACHE_ID = '0,
    parameter int MSHR_ENTRIES = cache_pkg::NUM_MSHR_ENTRY,
    parameter int CACHE_SETS   = cache_pkg::NUM_SETS,
    parameter int DATA_DEPTH   = cache_pkg::DATA_SRAM_DEPTH
) (
    input  logic                        i_clk,
    input  logic                        i_rst_n,

    input  cpu_req_t                    i_cpu_req,
    input  logic                        i_cpu_req_VALID,
    output logic                        o_cpu_req_READY,

    output cpu_rsp_t                    o_cpu_resp,
    output logic                        o_cpu_resp_VALID,
    input  logic                        i_cpu_resp_READY,

    output cpu_req_t                    o_lock_req,
    output logic                        o_lock_req_VALID,
    input  logic                        i_lock_req_READY,

    input  cpu_rsp_t                    i_lock_rsp,
    input  logic                        i_lock_rsp_VALID,
    output logic                        o_lock_rsp_READY,

    output coh_req_t                    o_coh_req,
    output logic                        o_coh_req_VALID,
    input  logic                        i_coh_req_READY,

    input  coh_rsp_t                    i_coh_rsp,
    input  logic                        i_coh_rsp_VALID,
    output logic                        o_coh_rsp_READY,

    input  coh_snp_t                    i_coh_snp,
    input  logic                        i_coh_snp_VALID,
    output logic                        o_coh_snp_READY,

    output coh_ack_t                    o_coh_ack,
    output logic                        o_coh_ack_VALID,
    input  logic                        i_coh_ack_READY,

    output cache_fwd_t                  o_inter_cache_data,
    output logic                        o_inter_cache_data_VALID,
    input  logic                        i_inter_cache_data_READY,

    input  cache_fwd_t                  i_inter_cache_data,
    input  logic                        i_inter_cache_data_VALID,
    output logic                        o_inter_cache_data_READY,

    output mem_req_t                    o_mem_req,
    output logic                        o_mem_req_VALID,
    input  logic                        i_mem_req_READY,

    input  mem_rsp_t                    i_mem_resp,
    input  logic                        i_mem_resp_VALID,
    output logic                        o_mem_resp_READY
);
    /*
     * Pipelines
     *
     * p0 CPU request:        i_cpu_req -> o_lock_req / p1A / hit response
     * p1A miss allocation:   p0 miss -> r_mshr allocation
     * p1B MSHR drain:        r_mshr -> o_coh_req and o_mem_req
     * p2 data/fill:          i_mem_resp or i_inter_cache_data -> cache SRAM / CPU response
     * p3 snoop:              i_coh_snp -> o_coh_ack and o_inter_cache_data
     * p4 coherence response: i_coh_rsp -> cache SRAM / CPU response
     * p5 coherence request:  p1B request state -> o_coh_req
     * p6 CPU response:       lock, hit, p2, or p4 completion -> o_cpu_resp
     */
`ifdef DUMB_COMPILER
    logic p1B_s1_update_VALID;
    logic p1B_s1_mshr_READY;
    logic p2_s2_select_VALID;
    logic p2_s2_select_READY;
    logic p4_s1_VALID;
    logic p4_s1_fill_VALID;
    logic p4_s1_fill_READY;
    logic p6_s1_READY;
`endif


    /*---  */
    logic [DATA_SRAM_ADDR_W-1:0]        w_data_sram_waddr;
    logic                               w_data_sram_wen;
    logic [CACHE_LINE_W-1:0]            w_data_sram_wdata;
    logic [CACHE_LINE_W/8-1:0]          w_data_sram_wstrb;
    logic                               p3_writeback_VALID;
    mem_req_t                           p3_writeback_req;
    logic [DATA_SRAM_ADDR_W-1:0]        w_data_sram_raddr;
    logic [CACHE_LINE_W-1:0]            w_data_sram_rdata;

    sram_1w1r #(
        .WORD_W         (CACHE_LINE_W     ),
        .DEPTH          (DATA_DEPTH  )
    ) u_data_sram (
        .i_clk          (i_clk            ),
        .i_req_waddr    (w_data_sram_waddr),
        .i_req_wen      (w_data_sram_wen  ),
        .i_req_wdata    (w_data_sram_wdata),
        .i_req_wstrb    (w_data_sram_wstrb),
        .i_req_raddr    (w_data_sram_raddr),
        .o_req_rdata    (w_data_sram_rdata)
    );

    typedef enum logic [1:0] {
        P0 = 2'b00,
        P2 = 2'b01,
        P3 = 2'b10,
        P4 = 2'b11
    } cache_pipe_e;

    cache_meta_t r_cache_meta   [CACHE_SETS-1:0      ][NUM_WAYS-1:0];
    mshr_entry_t r_mshr         [MSHR_ENTRIES-1:0];
    cache_pipe_e r_sram_winner;
    /*  ---*/

   

    /*
     * cacheable   -> sram&meta-> hit
     * cacheable   -> sram&meta -> miss
     * uncacheable -> lock port
     */
    /* p0: cpu request---  */
    logic                           p0_s3_VALID;
    logic                           p0_s3_READY;
    logic                           p0_s3_hit_READY;
    logic                           p0_s3_miss_READY;
    logic                           p0_s2_VALID;
    logic                           p0_s2_READY;
    logic                           p0_s1_VALID;
    logic                           p0_s1_READY;
    

    /* s0: decode req---  */
    cpu_req_t                       p0_s1_req;
    logic                           p0_s1_req_cacheable;

    logic                           p0_s0_line_pending;

    always_comb begin
        p0_s0_line_pending = 1'b0;

        if (i_cpu_req_VALID && (i_cpu_req.mem_type == MEM_WB)) begin
            p0_s0_line_pending |= p0_s1_VALID &&
                (get_cache_line_addr(i_cpu_req.addr) == get_cache_line_addr(p0_s1_req.addr));
            for (int mshr_id = 0; mshr_id < MSHR_ENTRIES; mshr_id++) begin
                p0_s0_line_pending |= (r_mshr[mshr_id].state != MSHR_FREE) &&
                    (get_cache_line_addr(i_cpu_req.addr) == get_cache_line_addr(r_mshr[mshr_id].addr));
            end
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s1_VALID         <= 1'b0;
            p0_s1_req           <= '0;
            p0_s1_req_cacheable <= 1'b0;
        end else if (p0_s1_READY) begin
            p0_s1_VALID <= i_cpu_req_VALID && !p0_s0_line_pending;
            if (i_cpu_req_VALID && !p0_s0_line_pending) begin
                p0_s1_req           <= i_cpu_req;
                p0_s1_req_cacheable <= (i_cpu_req.mem_type == MEM_WB) &&
                                       !is_bus_lock(i_cpu_req.op, i_cpu_req.mem_type,
                                  i_cpu_req.addr, i_cpu_req.size);
            end
        end
    end
    /*  ---s0 */

    /* s1: decide hit/miss & and parallely get the data out---  */
    logic [CACHE_LINE_W-1:0]        p0_s2_line;
    cpu_req_t                       p0_s2_req;
    logic [WAY_INDEX_W-1:0]         p0_s2_hit_way;
    logic                           p0_s2_hit;
    line_state_e                    p0_s2_line_state;
    logic                           p0_s2_req_cacheable;
    cache_addr_t                    p0_s2_decoded_addr;

    cache_addr_t                    p0_s1_decoded_addr;
    logic [WAY_INDEX_W-1:0]         p0_s1_hit_way;
    logic                           p0_s1_hit;
    logic                           p0_s1_line_match;
    line_state_e                    p0_s1_line_state;
    logic                           p0_s1_victim_invalid;
    logic [1:0]                     p0_s1_victim_rrpv;

    assign p0_s1_READY     = !p0_s1_VALID || p0_s2_READY;
    assign o_cpu_req_READY = p0_s1_READY && !p0_s0_line_pending;

    always_comb begin
        p0_s1_decoded_addr   = decode_cache_addr(p0_s1_req.addr);
        p0_s1_hit            = 1'b0;
        p0_s1_line_match     = 1'b0;
        p0_s1_hit_way        = '0;
        p0_s1_line_state     = r_cache_meta[p0_s1_decoded_addr.index][0].state;
        p0_s1_victim_invalid = !r_cache_meta[p0_s1_decoded_addr.index][0].valid;
        p0_s1_victim_rrpv    = r_cache_meta[p0_s1_decoded_addr.index][0].rrpv;

        for (int i = 0; i < NUM_WAYS; i++) begin
            if (!p0_s1_line_match &&
                r_cache_meta[p0_s1_decoded_addr.index][i].valid &&
                (r_cache_meta[p0_s1_decoded_addr.index][i].tag ==
                p0_s1_decoded_addr.tag) && p0_s1_req_cacheable &&
                cache_line_is_valid(r_cache_meta[p0_s1_decoded_addr.index][i].state)) begin

                p0_s1_line_match = 1'b1;
                p0_s1_hit_way    = WAY_INDEX_W'(i);
                p0_s1_line_state = r_cache_meta[p0_s1_decoded_addr.index][i].state;
`ifndef KILL_WRITABLE_STATE_CHECK
                p0_s1_hit        = !cpu_op_is_write(p0_s1_req.op) ||
                                   (p0_s1_line_state inside {LINE_M, LINE_E});
`else
                p0_s1_hit        = 1'b1;
`endif
            end
        end

        if (!p0_s1_line_match) begin
            for (int i = 0; i < NUM_WAYS; i++) begin
                if (!r_cache_meta[p0_s1_decoded_addr.index][i].valid &&
                    !p0_s1_victim_invalid) begin
                    p0_s1_hit_way    = WAY_INDEX_W'(i);
                    p0_s1_line_state = r_cache_meta[p0_s1_decoded_addr.index][i].state;
                    p0_s1_victim_invalid = 1'b1;
                end else if (!p0_s1_victim_invalid &&
                             (r_cache_meta[p0_s1_decoded_addr.index][i].rrpv >
                              p0_s1_victim_rrpv)) begin
                    p0_s1_hit_way      = WAY_INDEX_W'(i);
                    p0_s1_line_state   = r_cache_meta[p0_s1_decoded_addr.index][i].state;
                    p0_s1_victim_rrpv  = r_cache_meta[p0_s1_decoded_addr.index][i].rrpv;
                end
            end
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s2_VALID         <= 1'b0;
            p0_s2_hit_way       <= '0;
            p0_s2_hit           <= 1'b0;
            p0_s2_line_state    <= LINE_I;
            p0_s2_req           <= '0;
            p0_s2_req_cacheable <= 1'b0;
            p0_s2_decoded_addr  <= '0;
        end else if (p0_s2_READY) begin
            p0_s2_VALID <= p0_s1_VALID;
            if (p0_s1_VALID) begin
                p0_s2_req           <= p0_s1_req;
                p0_s2_hit_way       <= p0_s1_hit_way;
                p0_s2_hit           <= p0_s1_hit;
                p0_s2_line_state    <= p0_s1_line_state;
                p0_s2_req_cacheable <= p0_s1_req_cacheable;
                p0_s2_decoded_addr  <= p0_s1_decoded_addr;
            end
        end
    end
    /*  ---s1 */

    /* s2: entry to p2 or lock port---  */
    logic [CACHE_LINE_W-1:0]        p0_s3_line;
    cpu_req_t                       p0_s3_req;
    logic [WAY_INDEX_W-1:0]         p0_s3_way;
    logic                           p0_s3_hit;
    line_state_e                    p0_s3_line_state;
    logic                           p0_s3_req_cacheable;
    cache_addr_t                    p0_s3_decoded_addr;
    logic [CACHE_LINE_W-1:0]        p0_s3_commit_line;
    logic [XLEN-1:0]                p0_s3_old_data;


    assign p0_s2_READY  = !p0_s2_VALID ||
                         ((r_sram_winner == P0) && p0_s3_READY);
    assign  p0_s2_line  = w_data_sram_rdata;

    
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s3_VALID         <= 1'b0;
            p0_s3_line          <= '0;
            p0_s3_req           <= '0;
            p0_s3_way           <= '0;
            p0_s3_hit           <= 1'b0;
            p0_s3_line_state    <= LINE_I;
            p0_s3_req_cacheable <= 1'b0;
            p0_s3_decoded_addr  <= '0;
        end else if (p0_s3_READY) begin
            p0_s3_VALID <= p0_s2_VALID && (r_sram_winner == P0);
            if (p0_s2_VALID && (r_sram_winner == P0)) begin
                p0_s3_line          <= p0_s2_line;
                p0_s3_req           <= p0_s2_req;
                p0_s3_way           <= p0_s2_hit_way;
                p0_s3_hit           <= p0_s2_hit;
                p0_s3_line_state    <= p0_s2_line_state;
                p0_s3_req_cacheable <= p0_s2_req_cacheable;
                p0_s3_decoded_addr  <= p0_s2_decoded_addr;
            end
        end
    end
    /*  ---s2 */

    always_comb begin
        if (!p0_s3_VALID) begin
            p0_s3_READY = 1'b1;
        end else if (!p0_s3_req_cacheable) begin
            p0_s3_READY = i_lock_req_READY;
        end else if (!p0_s3_hit) begin
            p0_s3_READY = p0_s3_miss_READY;
        end else begin
            p0_s3_READY = (r_sram_winner == P0) &&
                          p0_s3_hit_READY;
        end
    end

    always_comb begin
        p0_s3_commit_line = p0_s3_line;
        p0_s3_old_data    = p0_s3_line[
                            p0_s3_req.addr[BYTE_OFFSET_W-1:0]*8 +: XLEN];

        if (cpu_op_is_atomic(p0_s3_req.op)) begin
            p0_s3_commit_line = merge_cache_line(p0_s3_line, p0_s3_req.addr[BYTE_OFFSET_W-1:0],
                                     execute_atomic(p0_s3_req.op, p0_s3_old_data,
                                                                p0_s3_req.wdata, p0_s3_req.compare_data),
                                     p0_s3_req.wstrb );
        end else if (cpu_op_is_write(p0_s3_req.op)) begin
            p0_s3_commit_line = merge_cache_line(
                p0_s3_line, p0_s3_req.addr[BYTE_OFFSET_W-1:0],
                p0_s3_req.wdata, p0_s3_req.wstrb
            );
        end
    end

    assign o_lock_req       = p0_s3_req;
    assign o_lock_req_VALID = p0_s3_VALID && !p0_s3_req_cacheable;
    assign p0_s3_hit_READY  = p6_s1_READY && !i_lock_rsp_VALID;
    assign p0_s3_miss_READY = p1B_s1_mshr_READY && !p1B_s1_update_VALID;
    /*  ---p0 */


    /*
     * Services p0
     * handle mshr alloc
     */
    /* p1A: miss pipe---  */
    logic                           p1A_s0_VALID;

    /* s0: allocate mshr---  */
    logic [TRANS_ID_W-1:0]          p1A_s0_mshr_id;
    mshr_entry_t                    p1A_s0_mshr;

    always_comb begin
        p1A_s0_VALID   = 1'b0;
        p1A_s0_mshr_id = '0;
        p1A_s0_mshr    = '0;
        for (int mshr_id = 0; mshr_id < MSHR_ENTRIES; mshr_id++) begin
            if (!p1A_s0_VALID && (r_mshr[mshr_id].state == MSHR_FREE)) begin
                p1A_s0_VALID   = p0_s3_VALID && p0_s3_req_cacheable && !p0_s3_hit;
                p1A_s0_mshr_id = TRANS_ID_W'(mshr_id);
            end
        end

        p1A_s0_mshr.state         = MSHR_ACQUIRE;
        p1A_s0_mshr.cpu_id        = p0_s3_req.id;
        p1A_s0_mshr.op            = p0_s3_req.op;
        p1A_s0_mshr.addr          = p0_s3_req.addr;
        p1A_s0_mshr.wdata         = p0_s3_req.wdata;
        p1A_s0_mshr.wstrb         = p0_s3_req.wstrb;
        p1A_s0_mshr.compare_data  = p0_s3_req.compare_data;
        p1A_s0_mshr.way           = p0_s3_way;
        p1A_s0_mshr.line_data     = p0_s3_line;
        p1A_s0_mshr.victim_data   = p0_s3_line;
        p1A_s0_mshr.victim_addr   = {r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].tag,
                                    p0_s3_decoded_addr.index, {BYTE_OFFSET_W{1'b0}}};
        p1A_s0_mshr.victim_dirty  = r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].valid &&
                                    cache_line_is_dirty(p0_s3_line_state);
        p1A_s0_mshr.data_received = r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].valid &&
                                    (r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].tag ==
                                    p0_s3_decoded_addr.tag);

        if (r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].valid &&
            (r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].tag != p0_s3_decoded_addr.tag)) begin
            p1A_s0_mshr.state = MSHR_EVICT;
        end
    end
    /*  ---s0 */
    /* ---p1A */
    /*
     * interacts with p5 and p1B
     */
    /* p1B: mshr drain---  */
    logic                           p1B_s1_READY;
    logic                           p1B_s1_coh_req_VALID;
    logic                           p1B_s1_coh_req_READY;
    logic                           p1B_s1_mem_req_VALID;
`ifndef DUMB_COMPILER
    logic                           p1B_s1_update_VALID;
    logic                           p1B_s1_mshr_READY;
`endif
    logic                           p1B_s0_VALID;
    logic                           p1B_s1_VALID;
    

    /* s0: select and drain mshr---  */
    logic [TRANS_ID_W-1:0]          p1B_s1_mshr_id;
    mshr_entry_t                    p1B_s1_mshr;
    coh_req_t                       p1B_s1_coh_req;
    mem_req_t                       p1B_s1_mem_req;
    logic                           p1B_s1_coh_pending;
    logic                           p1B_s1_mem_pending;

    logic [TRANS_ID_W-1:0]          p1B_s0_mshr_id;
    mshr_entry_t                    p1B_s0_mshr;
    coh_req_t                       p1B_s0_coh_req;
    mem_req_t                       p1B_s0_mem_req;

    
    always_comb begin
        p1B_s0_VALID          = 1'b0;
        p1B_s0_mshr_id        = '0;
        p1B_s0_mshr           = '0;
        p1B_s0_coh_req        = '0;
        p1B_s0_mem_req        = '0;
        p1B_s1_coh_req_VALID  = p1B_s1_VALID && p1B_s1_coh_pending;
        p1B_s1_mem_req_VALID  = p1B_s1_VALID && p1B_s1_mem_pending;
        p1B_s1_update_VALID   = p1B_s1_VALID && !p1B_s1_coh_pending && !p1B_s1_mem_pending;
        o_mem_req             = p3_writeback_VALID ? p3_writeback_req : p1B_s1_mem_req;
        o_mem_req_VALID       = p3_writeback_VALID || p1B_s1_mem_req_VALID;

        for (int mshr_id = 0; mshr_id < MSHR_ENTRIES; mshr_id++) begin
            if (!p1B_s0_VALID && (!p1B_s1_VALID || (p1B_s1_mshr_id != TRANS_ID_W'(mshr_id))) &&
               ((r_mshr[mshr_id].state == MSHR_EVICT) ||
                (r_mshr[mshr_id].state == MSHR_ACQUIRE) ||
                (r_mshr[mshr_id].state == MSHR_MEMORY_WAIT))) begin

                p1B_s0_VALID   = 1'b1;
                p1B_s0_mshr_id = TRANS_ID_W'(mshr_id);
                p1B_s0_mshr    = r_mshr[mshr_id];
            end
        end

        p1B_s0_coh_req.addr         = (p1B_s0_mshr.state == MSHR_EVICT) ?
                                      p1B_s0_mshr.victim_addr : p1B_s0_mshr.addr;
        p1B_s0_coh_req.src_id       = CACHE_ID;
        p1B_s0_coh_req.trans_id     = p1B_s0_mshr_id;
        p1B_s0_coh_req.msg          = (p1B_s0_mshr.state == MSHR_EVICT) ?
                                      (p1B_s0_mshr.victim_dirty ? PUTM : PUTS) :
                                      (cpu_op_is_write(p1B_s0_mshr.op) ? GETM : GETS);
        p1B_s0_mem_req.wen          = p1B_s0_mshr.state == MSHR_EVICT;
        p1B_s0_mem_req.requester_id = CACHE_ID;
        p1B_s0_mem_req.trans_id     = p1B_s0_mshr_id;
        p1B_s0_mem_req.addr         = (p1B_s0_mshr.state == MSHR_EVICT) ?
                                      p1B_s0_mshr.victim_addr : p1B_s0_mshr.addr;
        p1B_s0_mem_req.wdata        = p1B_s0_mshr.victim_data;
        p1B_s0_mem_req.wstrb        = '1;
    end
    
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p1B_s1_VALID       <= 1'b0;
            p1B_s1_mshr_id     <= '0;
            p1B_s1_mshr        <= '0;
            p1B_s1_coh_req     <= '0;
            p1B_s1_mem_req     <= '0;
            p1B_s1_coh_pending <= 1'b0;
            p1B_s1_mem_pending <= 1'b0;
        end else if (p1B_s1_READY) begin
            p1B_s1_VALID <= p1B_s0_VALID;
            if (p1B_s0_VALID) begin
                p1B_s1_mshr_id     <= p1B_s0_mshr_id;
                p1B_s1_mshr        <= p1B_s0_mshr;
                p1B_s1_coh_req     <= p1B_s0_coh_req;
                p1B_s1_mem_req     <= p1B_s0_mem_req;
                p1B_s1_coh_pending <= p1B_s0_mshr.state != MSHR_MEMORY_WAIT;
                p1B_s1_mem_pending <= (p1B_s0_mshr.state == MSHR_MEMORY_WAIT) ||
                                      ((p1B_s0_mshr.state == MSHR_EVICT) &&
                                       p1B_s0_mshr.victim_dirty);
            end
        end else begin
            if (p1B_s1_coh_req_VALID && p1B_s1_coh_req_READY) begin
                p1B_s1_coh_pending <= 1'b0;
            end
            if (p1B_s1_mem_req_VALID && !p3_writeback_VALID && i_mem_req_READY) begin
                p1B_s1_mem_pending <= 1'b0;
            end
        end
    end
    /*  ---s0 */
    assign p1B_s1_READY         = !p1B_s1_VALID || (p1B_s1_update_VALID && p1B_s1_mshr_READY);
    assign p1B_s1_coh_req_READY = !o_coh_req_VALID || i_coh_req_READY;
    assign p1B_s1_mshr_READY    = !(p2_s2_select_VALID && p2_s2_select_READY) &&
                                  !p4_s1_VALID;
    /* ---p1B */


    /*
     * handle data forward resp, mem resp ports, and interact with p5
     * service the cpu resp port
     * mshr dealloc is also done here
     */
    /* p2: hit pipe---  */
`ifndef DUMB_COMPILER
    logic                           p2_s2_select_VALID;
    logic                           p2_s2_select_READY;
`endif
    logic                           p2_s2_fill_VALID;
    logic                           p2_s2_fill_READY;
    logic                           p2_s0_VALID       [1:0];
    logic                           p2_s0_READY       [1:0];
    logic                           p2_s1_VALID       [1:0];
    logic                           p2_s1_READY       [1:0];
    logic                           p2_s2_VALID       [1:0];
    logic                           p2_s2_READY       [1:0];
    

    /* s0: access response ports---  */
    logic [TRANS_ID_W-1:0]          p2_s1_mshr_id     [1:0];
    logic [CACHE_LINE_W-1:0]        p2_s1_line        [1:0];

    logic [TRANS_ID_W-1:0]          p2_s0_mshr_id     [1:0];
    logic [CACHE_LINE_W-1:0]        p2_s0_line        [1:0];


    always_comb begin
        for (int lane = 0; lane < 2; lane++) begin
            p2_s0_VALID[lane]   = 1'b0;
            p2_s0_READY[lane]   = p2_s1_READY[lane];
            p2_s0_mshr_id[lane] = '0;
            p2_s0_line[lane]    = '0;
        end

        p2_s0_VALID[0]   = i_mem_resp_VALID && r_mshr[i_mem_resp.trans_id].grant_received;
        p2_s0_READY[0]   = p2_s1_READY[0] &&
                           (r_mshr[i_mem_resp.trans_id].grant_received ||
                            (r_mshr[i_mem_resp.trans_id].state == MSHR_FREE));
        p2_s0_mshr_id[0] = i_mem_resp.trans_id;
        p2_s0_line[0]    = i_mem_resp.rdata;
        p2_s0_VALID[1]   = i_inter_cache_data_VALID && !i_mem_resp_VALID &&
                           r_mshr[i_inter_cache_data.trans_id].grant_received &&
                           (r_mshr[i_inter_cache_data.trans_id].state != MSHR_MEMORY_WAIT) &&
                           (i_inter_cache_data.dst_id == CACHE_ID);
        p2_s0_READY[1]   = p2_s1_READY[1] && r_mshr[i_inter_cache_data.trans_id].grant_received;
        p2_s0_mshr_id[1] = i_inter_cache_data.trans_id;
        p2_s0_line[1]    = r_mshr[i_inter_cache_data.trans_id].line_data;

        for (int byte_id = 0; byte_id < CACHE_LINE_W/8; byte_id++) begin
            if (i_inter_cache_data.wstrb[byte_id]) begin
                p2_s0_line[1][byte_id*8 +: 8] =
                i_inter_cache_data.data[byte_id*8 +: 8];
            end
        end
        o_mem_resp_READY         = p2_s0_READY[0];
        o_inter_cache_data_READY = p2_s0_READY[1] && !i_mem_resp_VALID;
    end
    
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            for (int lane = 0; lane < 2; lane++) begin
                p2_s1_VALID  [lane] <= 1'b0;
                p2_s1_mshr_id[lane] <= '0;
                p2_s1_line   [lane] <= '0;
            end
        end else begin
            for (int lane = 0; lane < 2; lane++) begin
                if (p2_s1_READY[lane]) begin
                    p2_s1_VALID[lane] <= p2_s0_VALID[lane];
                    if (p2_s0_VALID[lane]) begin
                        p2_s1_mshr_id[lane] <= p2_s0_mshr_id[lane];
                        p2_s1_line[lane]    <= p2_s0_line[lane];
                    end
                end
            end
        end
    end
    /*  ---s0 */

    /* s1: resolve coherence grant---  */
    logic [TRANS_ID_W-1:0]          p2_s2_mshr_id     [1:0];
    mshr_entry_t                    p2_s2_mshr        [1:0];

    mshr_entry_t                    p2_s1_mshr        [1:0];

    for (genvar lane = 0; lane < 2; lane++) begin : g_p2_s1_resolve
        always_comb begin
            p2_s1_mshr[lane]               = r_mshr[p2_s1_mshr_id[lane]];
            p2_s1_mshr[lane].line_data     = p2_s1_line[lane];
            p2_s1_mshr[lane].data_received = 1'b1;
            p2_s1_READY[lane]              = !p2_s1_VALID[lane] || p2_s2_READY[lane];
        end
    end
    
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            for (int lane = 0; lane < 2; lane++) begin
                p2_s2_VALID[lane]   <= 1'b0;
                p2_s2_mshr_id[lane] <= '0;
                p2_s2_mshr[lane]    <= '0;
            end
        end else begin
            for (int lane = 0; lane < 2; lane++) begin
                if (p2_s2_READY[lane]) begin
                    p2_s2_VALID[lane] <= p2_s1_VALID[lane];

                    if (p2_s1_VALID[lane]) begin
                        p2_s2_mshr_id[lane] <= p2_s1_mshr_id[lane];
                        p2_s2_mshr[lane]    <= p2_s1_mshr[lane];
                    end
                end
            end
        end
    end
    /*  ---s1 */

    /* s2: update cache and respond---  */
    logic                           p2_s2_select;
    logic [CACHE_LINE_W-1:0]        p2_s2_commit_line;
    logic [XLEN-1:0]                p2_s2_old_data;

    always_comb begin
        p2_s2_READY        = '{default: 1'b0};
        p2_s2_select_VALID = p2_s2_VALID[0] || p2_s2_VALID[1];
        p2_s2_select       = !p2_s2_VALID[0] && p2_s2_VALID[1];
        for (int lane = 0; lane < 2; lane++) begin
            p2_s2_READY[lane] = !p2_s2_VALID[lane] ||
                                (p2_s2_select_READY && (r_sram_winner == P2) &&
                                p2_s2_select_VALID &&
                                (p2_s2_select == lane));
        end
        p2_s2_commit_line  = p2_s2_mshr[p2_s2_select].line_data;
        p2_s2_old_data     = p2_s2_commit_line[p2_s2_mshr[p2_s2_select].addr[BYTE_OFFSET_W-1:0]*8 +: XLEN];

        if (cpu_op_is_atomic(p2_s2_mshr[p2_s2_select].op)) begin
            p2_s2_commit_line = merge_cache_line(
                                p2_s2_commit_line,
                                p2_s2_mshr[p2_s2_select].addr[BYTE_OFFSET_W-1:0],
                                execute_atomic(
                                    p2_s2_mshr[p2_s2_select].op,
                                    p2_s2_old_data,
                                    p2_s2_mshr[p2_s2_select].wdata,
                                    p2_s2_mshr[p2_s2_select].compare_data),
                                p2_s2_mshr[p2_s2_select].wstrb
                                );
        end else if (cpu_op_is_write(p2_s2_mshr[p2_s2_select].op)) begin
            p2_s2_commit_line = merge_cache_line(
                                    p2_s2_commit_line,
                                    p2_s2_mshr[p2_s2_select].addr[BYTE_OFFSET_W-1:0],
                                    p2_s2_mshr[p2_s2_select].wdata,
                                    p2_s2_mshr[p2_s2_select].wstrb
                                );
        end
    end
    assign p2_s2_fill_VALID   = p2_s2_select_VALID &&
                                (r_mshr[p2_s2_mshr_id[p2_s2_select]].state != MSHR_FREE) &&
                                (r_sram_winner == P2);
    assign p2_s2_fill_READY   = p4_s1_fill_READY && !p4_s1_fill_VALID;
    assign p2_s2_select_READY = (r_sram_winner == P2) && p2_s2_fill_READY;
    /*  ---s2 */
    /* ---p2 */



    /*
     * handle the coh snoop port and the coh 
     * ack port and fwd req
     */
    /* p3: snoop pipe---  */
    logic                           p3_s1_VALID;
    logic                           p3_s1_READY;
    logic                           p3_s2_VALID;
    logic                           p3_s2_READY;
    logic                           p3_s2_fire;
    logic                           p3_s3_READY;

    /* s0: register snoop---  */
    coh_snp_t                       p3_s1_snp;

    logic                           p3_s0_line_pending;

    always_comb begin
        p3_s0_line_pending = 1'b0;
        if (i_coh_snp_VALID) begin
            p3_s0_line_pending |= p0_s1_VALID &&
                (get_cache_line_addr(i_coh_snp.addr) == get_cache_line_addr(p0_s1_req.addr));
            p3_s0_line_pending |= p0_s2_VALID &&
                (get_cache_line_addr(i_coh_snp.addr) == get_cache_line_addr(p0_s2_req.addr));
            p3_s0_line_pending |= p0_s3_VALID &&
                (get_cache_line_addr(i_coh_snp.addr) == get_cache_line_addr(p0_s3_req.addr));

            for (int mshr_id = 0; mshr_id < MSHR_ENTRIES; mshr_id++) begin
                p3_s0_line_pending |= (r_mshr[mshr_id].state != MSHR_FREE) &&
                    (get_cache_line_addr(i_coh_snp.addr) ==
                     get_cache_line_addr(r_mshr[mshr_id].addr));
            end
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p3_s1_snp   <= '0;
            p3_s1_VALID <= 1'b0;
        end else if (p3_s1_READY) begin
            p3_s1_VALID <= i_coh_snp_VALID && !p3_s0_line_pending;
            if (i_coh_snp_VALID && !p3_s0_line_pending) begin
                p3_s1_snp <= i_coh_snp;
            end
        end
    end
    /*  ---s0 */

    /* s1: lookup line and read sram---  */
    coh_snp_t                       p3_s2_snp;
    cache_addr_t                    p3_s2_addr;
    logic [WAY_INDEX_W-1:0]         p3_s2_way;
    logic                           p3_s2_hit;
    line_state_e                    p3_s2_state;
    logic [CACHE_LINE_W-1:0]        p3_s2_line;
    logic                           p3_s2_line_VALID;

    cache_addr_t                    p3_s1_addr;
    logic [WAY_INDEX_W-1:0]         p3_s1_way;
    logic                           p3_s1_hit;
    line_state_e                    p3_s1_state;

    
    assign p3_s1_READY = !p3_s1_VALID || p3_s2_READY;

    always_comb begin
        p3_s1_addr  = decode_cache_addr(p3_s1_snp.addr);
        p3_s1_way   = '0;
        p3_s1_hit   = 1'b0;
        p3_s1_state = LINE_I;

        for (int way = 0; way < NUM_WAYS; way++) begin
            if (!p3_s1_hit &&
                r_cache_meta[p3_s1_addr.index][way].valid &&
                cache_line_is_valid(r_cache_meta[p3_s1_addr.index][way].state) &&
                (r_cache_meta[p3_s1_addr.index][way].tag == p3_s1_addr.tag)) begin
            
                p3_s1_way   = WAY_INDEX_W'(way);
                p3_s1_hit   = 1'b1;
                p3_s1_state = r_cache_meta[p3_s1_addr.index][way].state;
            end
        end

        o_coh_snp_READY = p3_s1_READY && !p3_s0_line_pending;
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p3_s2_snp   <= '0;
            p3_s2_addr  <= '0;
            p3_s2_way   <= '0;
            p3_s2_hit   <= 1'b0;
            p3_s2_state <= LINE_I;
            p3_s2_line  <= '0;
            p3_s2_line_VALID <= 1'b0;
            p3_s2_VALID <= 1'b0;
        end else if (p3_s2_READY) begin
            p3_s2_VALID <= p3_s1_VALID;
            p3_s2_line_VALID <= 1'b0;
            if (p3_s1_VALID) begin
                p3_s2_snp   <= p3_s1_snp;
                p3_s2_addr  <= p3_s1_addr;
                p3_s2_way   <= p3_s1_way;
                p3_s2_hit   <= p3_s1_hit;
                p3_s2_state <= p3_s1_state;
            end
        end else if (!p3_s2_line_VALID) begin
            p3_s2_line       <= w_data_sram_rdata;
            p3_s2_line_VALID <= 1'b1;
`ifdef KILL_TRANSIENT_STATE_STABILITY
            p3_s2_snp.trans_id <= p3_s2_snp.trans_id + 1'b1;
`endif
        end
    end
    /*  ---s1 */

    /* s2: update state, acknowledge and forward---  */
    logic                           p3_s2_forward;
    line_state_e                    p3_s2_next_state;

    assign p3_s2_READY = !p3_s2_VALID ||
                         (p3_s2_line_VALID && (r_sram_winner == P3) && p3_s3_READY);
    assign p3_s2_fire  = p3_s2_VALID && p3_s2_line_VALID &&
                         (r_sram_winner == P3) && p3_s3_READY;

    always_comb begin
        p3_s2_forward    = p3_s2_hit &&  ((p3_s2_snp.msg == SNP_FWD_GETS) 
                         || (p3_s2_snp.msg == SNP_FWD_GETM));
        p3_s2_next_state = p3_s2_state;
        
        if (p3_s2_hit) begin
            p3_s2_next_state = get_next_line_state(p3_s2_state,
                        (p3_s2_snp.msg == SNP_FWD_GETS) ?
                               LINE_PROBE_FOR_READ : LINE_PROBE_FOR_WRITE);
        end

        p3_writeback_VALID        = p3_s2_VALID && p3_s2_line_VALID && p3_s2_hit &&
                                    (p3_s2_state == LINE_M) &&
                                    (p3_s2_snp.msg inside {SNP_INV, SNP_FWD_GETS, SNP_FWD_GETM});
        p3_writeback_req          = '0;
        p3_writeback_req.wen      = 1'b1;
        p3_writeback_req.addr     = p3_s2_snp.addr;
        p3_writeback_req.wdata    = p3_s2_line;
        p3_writeback_req.wstrb    = '1;
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            o_coh_ack                <= '0;
            o_coh_ack_VALID          <= 1'b0;
            o_inter_cache_data       <= '0;
            o_inter_cache_data_VALID <= 1'b0;

        end else if (p3_s3_READY) begin
            o_coh_ack_VALID          <= p3_s2_fire;
            o_inter_cache_data_VALID <= p3_s2_fire && p3_s2_forward;

            if (p3_s2_fire) begin
                o_coh_ack.addr              <= p3_s2_snp.addr;
                o_coh_ack.src_id            <= CACHE_ID;
                o_coh_ack.requester_id      <= p3_s2_snp.requester_id;
`ifndef INJECT_BAD_SNOOP_ACK
                o_coh_ack.trans_id          <= p3_s2_snp.trans_id;
`else
                o_coh_ack.trans_id          <= p3_s2_snp.trans_id + 1'b1;
`endif
                o_coh_ack.msg               <= !p3_s2_hit ? ACK_NOT_PRESENT :
                                               (p3_s2_forward ? ACK_DATA : ACK_INV);
                o_inter_cache_data.addr     <= p3_s2_snp.addr;
                o_inter_cache_data.src_id   <= CACHE_ID;
                o_inter_cache_data.dst_id   <= p3_s2_snp.requester_id;
`ifndef KILL_LATEST_COHERENT_WRITE
                o_inter_cache_data.data     <= p3_s2_line;
`else
                o_inter_cache_data.data     <= p3_s2_line ^ CACHE_LINE_W'(1);
`endif
                o_inter_cache_data.wstrb    <= '1;
                o_inter_cache_data.trans_id <= p3_s2_snp.trans_id;
            end

        end else begin
            if (o_coh_ack_VALID && i_coh_ack_READY) begin
                o_coh_ack_VALID <= 1'b0;
            end

            if (o_inter_cache_data_VALID && i_inter_cache_data_READY) begin
                o_inter_cache_data_VALID <= 1'b0;
            end
        end
    end
    /*  ---s2 */
    assign p3_s3_READY = (!o_coh_ack_VALID || i_coh_ack_READY) &&
                         (!p3_s2_forward   || !o_inter_cache_data_VALID ||
                          i_inter_cache_data_READY) &&
                         (!p3_writeback_VALID || i_mem_req_READY);
    /*  ---p3 */



    /*
     * handles coh_resp port
     */
    /* p4: coh resp pipe---  */
`ifndef DUMB_COMPILER
    logic                           p4_s1_VALID;
`endif
    logic                           p4_s1_READY;
`ifndef DUMB_COMPILER
    logic                           p4_s1_fill_VALID;
    logic                           p4_s1_fill_READY;
`endif

    /* s0---  */
    logic [TRANS_ID_W-1:0]          p4_s1_trans_id;
    mshr_entry_t                    p4_s1_mshr;
    logic [CACHE_LINE_W-1:0]        p4_s1_commit_line;
    logic [XLEN-1:0]                p4_s1_old_data;

    assign p4_s1_fill_READY = p0_s3_hit_READY &&
                              !(p0_s3_VALID && p0_s3_hit && (r_sram_winner == P0));

    always_comb begin
        p4_s1_commit_line = p4_s1_mshr.line_data;
        p4_s1_old_data    = p4_s1_mshr.line_data[p4_s1_mshr.addr[BYTE_OFFSET_W-1:0]*8 +: XLEN];

        if (cpu_op_is_atomic(p4_s1_mshr.op)) begin
            p4_s1_commit_line = merge_cache_line(
                p4_s1_mshr.line_data,
                p4_s1_mshr.addr[BYTE_OFFSET_W-1:0],
                execute_atomic(p4_s1_mshr.op, p4_s1_old_data,
                                          p4_s1_mshr.wdata, p4_s1_mshr.compare_data),
                p4_s1_mshr.wstrb
            );
        end else if (cpu_op_is_write(p4_s1_mshr.op)) begin
            p4_s1_commit_line = merge_cache_line(
                p4_s1_mshr.line_data,
                p4_s1_mshr.addr[BYTE_OFFSET_W-1:0],
                p4_s1_mshr.wdata, p4_s1_mshr.wstrb
            );
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p4_s1_VALID    <= 1'b0;
            p4_s1_trans_id <= '0;
            p4_s1_mshr     <= '0;

        end else if (o_coh_rsp_READY) begin
            p4_s1_VALID <= i_coh_rsp_VALID;

            if (i_coh_rsp_VALID) begin
                p4_s1_trans_id <= i_coh_rsp.trans_id;
                p4_s1_mshr     <= r_mshr[i_coh_rsp.trans_id];
                if (i_coh_rsp.memory_required) begin
                    p4_s1_mshr.state <= MSHR_MEMORY_WAIT;
                    p4_s1_mshr.data_received <= 1'b0;
                end

                case (i_coh_rsp.msg)
                    GRANT_S: begin
                        p4_s1_mshr.grant_state    <= LINE_S;
                        p4_s1_mshr.grant_received <= 1'b1;
                    end
                    GRANT_E: begin
                        p4_s1_mshr.grant_state    <= LINE_E;
                        p4_s1_mshr.grant_received <= 1'b1;
                    end
                    GRANT_M: begin
                        p4_s1_mshr.grant_state    <= LINE_M;
                        p4_s1_mshr.grant_received <= 1'b1;
                    end
                    RETRY: begin
                        p4_s1_mshr.state          <= MSHR_ACQUIRE;
                        p4_s1_mshr.grant_received <= 1'b0;
                    end
                endcase
            end
        end
    end
    /*  ---s0 */
    assign p4_s1_fill_VALID = p4_s1_VALID &&
                              (r_mshr[p4_s1_trans_id].state != MSHR_FREE) &&
                              p4_s1_mshr.grant_received &&
                              p4_s1_mshr.data_received && (r_sram_winner == P4);
    assign p4_s1_READY      = !p4_s1_VALID ||
                              (!(p2_s2_select_VALID && p2_s2_select_READY) &&
                              (!(p4_s1_mshr.grant_received && p4_s1_mshr.data_received) ||
                              ((r_sram_winner == P4) && p4_s1_fill_READY)));
    /*  ---p4 */



    /*
     * service p1B
     * owns the coh req port
     */
    /* p5: coh req pipe---  */
    /* s0---  */
    assign o_coh_rsp_READY  = p4_s1_READY;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            o_coh_req       <= '0;
            o_coh_req_VALID <= 1'b0;
        end else begin
            if (p1B_s1_coh_req_READY) begin
                o_coh_req_VALID <= p1B_s1_coh_req_VALID;
                if (p1B_s1_coh_req_VALID) begin
                    o_coh_req <= p1B_s1_coh_req;
                end
            end
        end
    end
    /* ---s0 */
    /*  ---p5 */


    /* p6: cpu response pipe---  */
    logic                           p6_s1_VALID;
`ifndef DUMB_COMPILER
    logic                           p6_s1_READY;
`endif

    /* s0---  */
    cpu_rsp_t                       p6_s1_resp;

    always_comb begin
        o_lock_rsp_READY = p6_s1_READY;
        o_cpu_resp       = p6_s1_resp;
        o_cpu_resp_VALID = p6_s1_VALID;
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p6_s1_VALID <= 1'b0;
            p6_s1_resp  <= '0;
        end else if (p6_s1_READY) begin
            p6_s1_VALID <= i_lock_rsp_VALID ||
                           (p0_s3_VALID && p0_s3_hit && (r_sram_winner == P0)) ||
                           (p4_s1_fill_VALID && p4_s1_fill_READY) ||
                           (p2_s2_fill_VALID && p2_s2_fill_READY);

            if (i_lock_rsp_VALID) begin
                p6_s1_resp <= i_lock_rsp;
            end else if (p0_s3_VALID && p0_s3_hit && (r_sram_winner == P0)) begin
                p6_s1_resp.id             <= p0_s3_req.id;
                p6_s1_resp.rdata          <= p0_s3_old_data;
                p6_s1_resp.atomic_success <= 1'b1;
            end else if (p4_s1_fill_VALID && p4_s1_fill_READY) begin
                p6_s1_resp.id             <= p4_s1_mshr.cpu_id;
                p6_s1_resp.rdata          <= p4_s1_old_data;
                p6_s1_resp.atomic_success <= 1'b1;
            end else if (p2_s2_fill_VALID && p2_s2_fill_READY) begin
                p6_s1_resp.id             <= p2_s2_mshr[p2_s2_select].cpu_id;
                p6_s1_resp.rdata          <= p2_s2_old_data;
                p6_s1_resp.atomic_success <= 1'b1;
            end
        end
    end
    /* ---s0 */
    assign p6_s1_READY = !p6_s1_VALID || i_cpu_resp_READY;
    /*  ---p6 */
    

    /* shared cache line state update---  */
    always @(posedge i_clk) begin
        automatic cache_addr_t line_addr;

        if (!i_rst_n) begin
            for (int set = 0; set < CACHE_SETS; set++) begin
                for (int way = 0; way < NUM_WAYS; way++) begin
                    r_cache_meta[set][way].valid <= 1'b0;
                    r_cache_meta[set][way].tag   <= '0;
                    r_cache_meta[set][way].rrpv  <= '0;
                    r_cache_meta[set][way].state <= get_next_line_state(
                    r_cache_meta[set][way].state, LINE_RESET);
                end
            end
        end else begin
            if (p3_s2_fire && p3_s2_hit) begin
                r_cache_meta[p3_s2_addr.index][p3_s2_way].valid <= p3_s2_next_state != LINE_I;
                r_cache_meta[p3_s2_addr.index][p3_s2_way].tag   <= p3_s2_addr.tag;
                r_cache_meta[p3_s2_addr.index][p3_s2_way].state <= p3_s2_next_state;
            end else if (p0_s3_VALID && p0_s3_hit && (r_sram_winner == P0) &&
                         cpu_op_is_write(p0_s3_req.op)) begin
                r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].state <=
                get_next_line_state(
                r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].state, LINE_WRITE);
            end else if (p4_s1_VALID && p4_s1_READY && p4_s1_mshr.grant_received && p4_s1_mshr.data_received) begin
                line_addr = decode_cache_addr(p4_s1_mshr.addr);
                r_cache_meta[line_addr.index][p4_s1_mshr.way].valid <= 1'b1;
                r_cache_meta[line_addr.index][p4_s1_mshr.way].tag   <= line_addr.tag;
                r_cache_meta[line_addr.index][p4_s1_mshr.way].state <=
                cpu_op_is_write(p4_s1_mshr.op) ? get_next_line_state(LINE_I, LINE_WRITE) :
                ((p4_s1_mshr.grant_state == LINE_E) ? get_next_line_state(LINE_I, LINE_READ_WRITABLE_CLEAN) :
                get_next_line_state(LINE_I, LINE_READ_RO_CLEAN));
            end else if (p2_s2_select_VALID && p2_s2_READY[p2_s2_select]) begin
                line_addr = decode_cache_addr(p2_s2_mshr[p2_s2_select].addr);
                r_cache_meta[line_addr.index][p2_s2_mshr[p2_s2_select].way].valid <= 1'b1;
                r_cache_meta[line_addr.index][p2_s2_mshr[p2_s2_select].way].tag   <= line_addr.tag;
                r_cache_meta[line_addr.index][p2_s2_mshr[p2_s2_select].way].state <=
                cpu_op_is_write(p2_s2_mshr[p2_s2_select].op) ? get_next_line_state(LINE_I, LINE_WRITE) :
                ((p2_s2_mshr[p2_s2_select].grant_state == LINE_E) ?
                get_next_line_state(LINE_I, LINE_READ_WRITABLE_CLEAN) :
                get_next_line_state(LINE_I, LINE_READ_RO_CLEAN));
            end else if (p1B_s1_update_VALID && p1B_s1_mshr_READY) begin
                line_addr = decode_cache_addr((p1B_s1_mshr.state == MSHR_EVICT) ? p1B_s1_mshr.victim_addr : p1B_s1_mshr.addr);
                r_cache_meta[line_addr.index][p1B_s1_mshr.way].valid <=
                p1B_s1_mshr.state != MSHR_EVICT;
                r_cache_meta[line_addr.index][p1B_s1_mshr.way].tag   <= line_addr.tag;
                r_cache_meta[line_addr.index][p1B_s1_mshr.way].state <=
                (p1B_s1_mshr.state == MSHR_EVICT) ? get_next_line_state(
                r_cache_meta[line_addr.index][p1B_s1_mshr.way].state,
                p1B_s1_mshr.victim_dirty ? LINE_WB_INVD : LINE_INVD) :
                get_acquire_line_state(
                r_cache_meta[line_addr.index][p1B_s1_mshr.way].state,
                p1B_s1_mshr.op);
                end
        end
    end
    /*  ---shared cache line state update */



    /* shared sram read/write---  */
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            r_sram_winner <= P0;
        end else begin
            if (p2_s2_select_VALID) begin
                r_sram_winner <= P2;
            end else if (i_coh_snp_VALID || p3_s1_VALID || p3_s2_VALID) begin
                r_sram_winner <= P3;
            end else if (i_coh_rsp_VALID || p4_s1_VALID) begin
                r_sram_winner <= P4;
            end else begin
                r_sram_winner <= P0;
            end
        end
    end

    always_comb begin
        w_data_sram_raddr = '0;
        w_data_sram_waddr = '0;
        w_data_sram_wen   = 1'b0;
        w_data_sram_wdata = '0;
        w_data_sram_wstrb = '0;

        case (r_sram_winner)
            P2: begin
                w_data_sram_waddr = get_data_sram_addr(
                                    decode_cache_addr(p2_s2_mshr[p2_s2_select].addr).index,
                                    p2_s2_mshr[p2_s2_select].way);
                w_data_sram_wen   = p2_s2_select_VALID && p2_s2_READY[p2_s2_select];
                w_data_sram_wdata = p2_s2_commit_line;
                w_data_sram_wstrb = '1;
            end
            P0: begin
                w_data_sram_raddr = get_data_sram_addr(p0_s1_decoded_addr.index, p0_s1_hit_way);
                w_data_sram_waddr = get_data_sram_addr(p0_s3_decoded_addr.index, p0_s3_way);
                w_data_sram_wen   = p0_s3_VALID && p0_s3_hit &&
                                    cpu_op_is_write(p0_s3_req.op);
                w_data_sram_wdata = p0_s3_commit_line;
                w_data_sram_wstrb = '1;
            end
            P3: begin
                w_data_sram_raddr = get_data_sram_addr(p3_s1_addr.index, p3_s1_way);
            end
            P4: begin
                w_data_sram_waddr = get_data_sram_addr(
                                    decode_cache_addr(p4_s1_mshr.addr).index, p4_s1_mshr.way);
                w_data_sram_wen   = p4_s1_VALID && p4_s1_READY &&
                                    p4_s1_mshr.grant_received && p4_s1_mshr.data_received;
                w_data_sram_wdata = p4_s1_commit_line;
                w_data_sram_wstrb = '1;
            end
            default: begin
            end
        endcase
    end
    /* ---shared sram read/write */



    /* shared rrpv update---  */
    always @(posedge i_clk) begin
        automatic cache_addr_t fill_addr;
        automatic logic [2:0]  aged_rrpv;

        if (!i_rst_n) begin
            for (int set = 0; set < CACHE_SETS; set++) begin
                for (int way = 0; way < NUM_WAYS; way++) begin
                    r_cache_meta[set][way].rrpv <= 2'b11;
                end
            end
        end else begin
            if (p0_s2_VALID && p0_s2_READY && p0_s2_hit) begin
                r_cache_meta[p0_s2_decoded_addr.index][p0_s2_hit_way].rrpv <= 2'b00;
            end

            if (p1A_s0_VALID && p0_s3_miss_READY &&
                r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].valid) begin
                for (int way = 0; way < NUM_WAYS; way++) begin
                    aged_rrpv = r_cache_meta[p0_s3_decoded_addr.index][way].rrpv +
                                (3 - r_cache_meta[p0_s3_decoded_addr.index][p0_s3_way].rrpv);
                    r_cache_meta[p0_s3_decoded_addr.index][way].rrpv <=
                    aged_rrpv[2] ? 2'b11 : aged_rrpv[1:0];
                end
            end

            if (p2_s2_select_VALID && p2_s2_READY[p2_s2_select]) begin
                fill_addr = decode_cache_addr(p2_s2_mshr[p2_s2_select].addr);
                r_cache_meta[fill_addr.index] [p2_s2_mshr[p2_s2_select].way].rrpv <= 2'b10;
            end
        end
    end
    /*  ---shared rrpv update */



    /* shared mshr update---  */
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            for (int i = 0; i < MSHR_ENTRIES; i++) begin
                r_mshr[i] <= '0;
            end
        end else begin
            if (p2_s2_select_VALID && p2_s2_select_READY) begin
                if (p2_s2_READY[p2_s2_select]) begin
                    r_mshr[p2_s2_mshr_id[p2_s2_select]]       <= '0;
                    r_mshr[p2_s2_mshr_id[p2_s2_select]].state <= MSHR_FREE;
                end
            end else begin
                if (p4_s1_VALID) begin
                    if (p4_s1_mshr.grant_received && p4_s1_mshr.data_received) begin
                        if ((r_sram_winner == P4) && p4_s1_fill_READY) begin

                            r_mshr[p4_s1_trans_id]       <= '0;
                            r_mshr[p4_s1_trans_id].state <= MSHR_FREE;
                        end
                    end else begin
                        r_mshr[p4_s1_trans_id] <= p4_s1_mshr;
                    end
                end else if (p1B_s1_update_VALID) begin
                    r_mshr[p1B_s1_mshr_id]       <= p1B_s1_mshr;
                    r_mshr[p1B_s1_mshr_id].state <= (p1B_s1_mshr.state == MSHR_EVICT) ?  MSHR_ACQUIRE : MSHR_WAIT;
                end else if (p1A_s0_VALID) begin
                    r_mshr[p1A_s0_mshr_id] <= p1A_s0_mshr;
                end
            end
        end
    end
    /* ---shared mshr update */

endmodule: cache
