/*
 * directory.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. o_cpu_req_allow maintains the invaraint
 *    at most one outstanding transaction per cache line
 *    
 *    - this limits the concurruency and is too strong
 *      than it should be but it keep the design simpler
 *    - TODO: something little less strong (one ownership change per line)
 *      e.g.
 *          GETM / store / AMO / SC / eviction -> exclusive line reservation
 *          GETS / load                        -> shared read tracking
 */

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;
import directory_pkg::*;

module directory #(
    parameter int NUM_DIR_ENTRIES        = MEM_SIZE,
    parameter int NUM_SCOREBOARD_ENTRIES = NUM_LINE_RESERVATIONS
) (
    input  logic                         i_clk,
    input  logic                         i_rst_n,

    input  cpu_req_t                     i_cpu_req                 [NUM_CACHES-1:0],
    input  logic                         i_cpu_req_VALID           [NUM_CACHES-1:0],
    input  logic                         i_cpu_req_READY           [NUM_CACHES-1:0],
    output logic                         o_cpu_req_allow           [NUM_CACHES-1:0],
    input  cpu_rsp_t                     i_cpu_rsp                 [NUM_CACHES-1:0],
    input  logic                         i_cpu_rsp_VALID           [NUM_CACHES-1:0],
    input  logic                         i_cpu_rsp_READY           [NUM_CACHES-1:0],

    input  cpu_req_t                     i_lock_req                [NUM_CACHES-1:0],
    input  logic                         i_lock_req_VALID          [NUM_CACHES-1:0],
    output logic                         o_lock_req_READY          [NUM_CACHES-1:0],
    output cpu_rsp_t                     o_lock_rsp                [NUM_CACHES-1:0],
    output logic                         o_lock_rsp_VALID          [NUM_CACHES-1:0],
    input  logic                         i_lock_rsp_READY          [NUM_CACHES-1:0],

    input  coh_req_t                     i_cache_req               [NUM_CACHES-1:0],
    input  logic                         i_cache_req_VALID         [NUM_CACHES-1:0],
    output logic                         o_cache_req_READY         [NUM_CACHES-1:0],

    output coh_rsp_t                     o_cache_rsp               [NUM_CACHES-1:0],
    output logic                         o_cache_rsp_VALID         [NUM_CACHES-1:0],
    input  logic                         i_cache_rsp_READY         [NUM_CACHES-1:0],

    output coh_snp_t                     o_cache_snp               [NUM_CACHES-1:0],
    output logic                         o_cache_snp_VALID         [NUM_CACHES-1:0],
    input  logic                         i_cache_snp_READY         [NUM_CACHES-1:0],

    input  coh_ack_t                     i_cache_ack               [NUM_CACHES-1:0],
    input  logic                         i_cache_ack_VALID         [NUM_CACHES-1:0],
    output logic                         o_cache_ack_READY         [NUM_CACHES-1:0],

    output mem_req_t                     o_lock_mem_req,
    output logic                         o_lock_mem_req_VALID,
    input  logic                         i_lock_mem_req_READY,
    input  mem_rsp_t                     i_lock_mem_rsp,
    input  logic                         i_lock_mem_rsp_VALID,
    output logic                         o_lock_mem_rsp_READY
`ifdef FORMAL_SCOREBOARD
   ,output logic                         o_formal_slot_0_valid
   ,output logic                         o_formal_slot_1_valid
   ,output logic                         o_formal_slot_addr_equal
`endif
`ifdef FORMAL_EXCLUSIVE_OWNER
   ,input  logic                         i_formal_owner_valid
   ,input  logic [NUM_CACHES-1:0]        i_formal_sharers
   ,output logic                         o_formal_exclusive_violation
`endif
`ifdef FORMAL_ACK_MATCH
   ,input  logic                         i_formal_wait_valid
   ,input  logic [CACHE_ID_W-1:0]        i_formal_snoop_target
   ,input  coh_req_t                     i_formal_snoop_req
   ,output logic                         o_formal_ack_match
   ,output logic                         o_formal_snoop_complete
   ,output logic                         o_formal_conflicting_grant
`endif
`ifdef FORMAL_STABILITY
   ,input  logic                         i_formal_rsp_valid
   ,input  coh_rsp_t                     i_formal_rsp
   ,input  logic                         i_formal_rsp_ready
   ,output logic                         o_formal_rsp_next_valid
   ,output coh_rsp_t                     o_formal_rsp_next
   ,input  logic                         i_formal_snp_valid
   ,input  coh_snp_t                     i_formal_snp
   ,input  logic                         i_formal_snp_ready
   ,output logic                         o_formal_snp_next_valid
   ,output coh_snp_t                     o_formal_snp_next
`endif
);
`ifdef KILL_STALLED_OUTPUT_STABILITY
    logic [TRANS_ID_W-1:0] r_kill_stability_counter;
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) r_kill_stability_counter <= '0;
        else          r_kill_stability_counter <= r_kill_stability_counter + 1'b1;
    end
`endif

`ifdef DUMB_COMPILER
    logic                          p0_s5_VALID;
    logic [CACHE_ID_W-1:0]         p0_s5_winner;
    coh_req_t                      p0_s2_req;
`endif


    /*---  */
    dir_entry_t                    r_dir    [NUM_DIR_ENTRIES-1:0];
    bus_lock_state_e               s_this;
    bus_lock_state_e               s_next;
    cpu_req_t                      r_bus_req;
    logic [PORT_W-1:0]             r_owner;
    logic                          w_lock_snp_VALID;
    logic [CACHE_ID_W-1:0]         w_lock_snp_target;
    logic [PHY_ADDR_W-1:0]         w_lock_snp_addr;
    logic                          w_lock_ack_match;
    /*  ---*/


    /* p0: request/grant pipeline---  */
`ifndef DUMB_COMPILER
    logic                          p0_s5_VALID;
`endif
    logic                          p0_s5_READY;
    logic                          p0_s4_VALID;
    logic                          p0_s4_READY;
    logic                          p0_s3_VALID;
    logic                          p0_s3_READY;
    logic                          p0_s2_VALID;
    logic                          p0_s2_READY;
    logic                          p0_s1_VALID;
    logic                          p0_s1_READY;
    logic                          p0_s1_grant_VALID;
    
    
    logic [CACHE_ID_W-1:0]         p0_s2_winner;
    logic [DIR_INDEX_W-1:0]        p0_s2_index;

    /* s0: arbitrate and register request---  */
    coh_req_t                      p0_s1_req;
    logic [CACHE_ID_W-1:0]         p0_s1_winner;
    logic [DIR_INDEX_W-1:0]        p0_s1_index;

    logic [CACHE_ID_W-1:0]         p0_s0_winner;
    logic                          p0_s0_entry_pending [NUM_CACHES-1:0];
    logic                          p0_s0_lock_pending;

    always_comb begin
        p0_s0_entry_pending = '{default: 1'b0};
        p0_s0_lock_pending  = 1'b0;
        o_cache_req_READY   = '{default: 1'b0};

        for (int i = 0; i < NUM_CACHES; i++) begin
            p0_s0_lock_pending |= i_lock_req_VALID[i];
            p0_s0_entry_pending[i] =
                (p0_s1_VALID &&
                 (i_cache_req[i].addr[BYTE_OFFSET_W +: DIR_INDEX_W] ==
                  p0_s1_index)) ||
                (p0_s2_VALID &&
                 (i_cache_req[i].addr[BYTE_OFFSET_W +: DIR_INDEX_W] ==
                  p0_s2_index));
        end
`ifndef KILL_FAIRNESS
        p0_s0_winner = '0;
        for (int off = NUM_CACHES; off > 0; off--) begin
            if (i_cache_req_VALID[(p0_s2_winner + off) % NUM_CACHES] &&
                !p0_s0_entry_pending[(p0_s2_winner + off) % NUM_CACHES]) begin
                p0_s0_winner = CACHE_ID_W'((p0_s2_winner + off) % NUM_CACHES);
            end
        end
`else
        p0_s0_winner = i_cpu_rsp_READY[0];
`endif
`ifdef KILL_JUSTICE
        p0_s0_winner = CACHE_ID_W'(1);
`endif
        for (int i = 0; i < NUM_CACHES; i++) begin
`ifndef KILL_MUEXC_CACHE_REQ_ACCEPT
            o_cache_req_READY[i] = p0_s1_READY && !p0_s0_entry_pending[i] &&
                                   (p0_s0_winner == CACHE_ID_W'(i))
`ifndef KILL_REQUEST_TYPE_FAIRNESS
                                   && !p0_s0_lock_pending
`endif
                                   ;
`else
            o_cache_req_READY[i] = p0_s1_READY && !p0_s0_entry_pending[i];
`endif
        end
    end

`ifndef FORMAL_MUEXC
`ifndef FORMAL_SCOREBOARD
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s1_VALID  <= 1'b0;
            p0_s1_req    <= '0;
            p0_s1_winner <= '0;
            p0_s1_index  <= '0;

        end else if (p0_s1_READY) begin
            p0_s1_VALID <= i_cache_req_VALID[p0_s0_winner] &&
                           !p0_s0_entry_pending[p0_s0_winner]
`ifndef KILL_REQUEST_TYPE_FAIRNESS
                           && !p0_s0_lock_pending
`endif
                           ;
            if (i_cache_req_VALID[p0_s0_winner] &&
                !p0_s0_entry_pending[p0_s0_winner]
`ifndef KILL_REQUEST_TYPE_FAIRNESS
                && !p0_s0_lock_pending
`endif
                ) begin
                p0_s1_req    <= i_cache_req[p0_s0_winner];
                p0_s1_winner <= p0_s0_winner;
                p0_s1_index  <= i_cache_req[p0_s0_winner].addr[BYTE_OFFSET_W +: DIR_INDEX_W];
            end
        end
    end
    /*  ---s0 */

`ifndef FORMAL_FAIR
    /* s1: lookup and route request---  */
`ifndef DUMB_COMPILER
    coh_req_t                      p0_s2_req;
`endif
    coh_rsp_e                      p0_s2_grant;
    logic                          p0_s2_memory_required;
    logic                          p0_s2_from_snoop;
    logic [CACHE_ID_W-1:0]         p0_s2_snoop_target;
    
    dir_addr_t                     p0_s1_addr;
    dir_entry_t                    p0_s1_entry;
    logic                          p0_s1_hit;
    logic                          p0_s1_other_owner;
    logic [NUM_CACHES-1:0]         p0_s1_other_sharers;
    logic                          p0_s1_needs_snoop;
    logic [NUM_CACHES-1:0]         p0_s1_snoop_candidates;
    logic [CACHE_ID_W-1:0]         p0_s1_snoop_target;
    coh_snp_e                      p0_s1_snoop_msg;
    coh_rsp_e                      p0_s1_grant;
    logic                          p0_s1_memory_required;
    coh_req_t                      p0_s1_grant_req;
    logic [CACHE_ID_W-1:0]         p0_s1_grant_winner;
    coh_rsp_e                      p0_s1_grant_msg;
    logic                          p0_s1_grant_memory_required;
    logic                          p0_s1_grant_from_snoop;
    logic [CACHE_ID_W-1:0]         p0_s1_grant_snoop_target;


    assign p0_s1_READY = !p0_s1_VALID || p0_s2_READY;

    always_comb begin
        p0_s1_addr             = decode_dir_addr(p0_s1_req.addr);
        p0_s1_entry            = r_dir[p0_s1_addr.index];
        p0_s1_hit              = dir_entry_is_hit(p0_s1_entry, p0_s1_addr);
        p0_s1_other_owner      = dir_entry_has_other_owner(p0_s1_entry, p0_s1_winner);
        p0_s1_other_sharers    = get_other_sharers(p0_s1_entry, p0_s1_winner);
        p0_s1_needs_snoop      = 1'b0;
        p0_s1_snoop_candidates = '0;
        p0_s1_snoop_target     = '0;
        p0_s1_snoop_msg        = SNP_INV;
        p0_s1_grant            = (p0_s1_req.msg == GETM) ? GRANT_M : GRANT_S;
        p0_s1_memory_required  = 1'b0;

        if (!p0_s1_hit) begin
            p0_s1_grant           = (p0_s1_req.msg == GETM) ? GRANT_M : GRANT_E;
            p0_s1_memory_required = 1'b1;

        end else if (p0_s1_req.msg == GETM) begin
            p0_s1_snoop_candidates = p0_s1_other_owner ? (NUM_CACHES'(1) << p0_s1_entry.owner) : p0_s1_other_sharers;
            p0_s1_needs_snoop      = p0_s1_snoop_candidates != '0;
            p0_s1_snoop_msg        = p0_s1_other_owner ? SNP_FWD_GETM : SNP_INV;
            p0_s1_memory_required  = !p0_s1_needs_snoop && !p0_s1_entry.sharers[p0_s1_winner];

        end else if (p0_s1_other_owner) begin
            p0_s1_snoop_candidates = NUM_CACHES'(1) << p0_s1_entry.owner;
            p0_s1_needs_snoop      = 1'b1;
            p0_s1_snoop_msg        = SNP_FWD_GETS;
        end else begin
            p0_s1_memory_required = 1'b1;
        end

        for (int off = NUM_CACHES; off > 0; off--) begin
            if (p0_s1_snoop_candidates[(p0_s5_winner + off) % NUM_CACHES]) begin
                p0_s1_snoop_target = CACHE_ID_W'((p0_s5_winner + off) % NUM_CACHES);
            end
        end

        p0_s1_grant_VALID           = p0_s1_VALID && !p0_s1_needs_snoop &&
                                     !(p0_s1_req.msg inside {PUTS, PUTM});
        p0_s1_grant_req             = p0_s1_req;
        p0_s1_grant_winner          = p0_s1_winner;
        p0_s1_grant_msg             = p0_s1_grant;
        p0_s1_grant_memory_required = p0_s1_memory_required;
        p0_s1_grant_from_snoop      = 1'b0;
        p0_s1_grant_snoop_target    = '0;

    end

    /* s2: register direct grant---  */
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s2_VALID           <= 1'b0;
            p0_s2_req             <= '0;
            p0_s2_winner          <= '0;
            p0_s2_index           <= '0;
            p0_s2_grant           <= GRANT_S;
            p0_s2_memory_required <= 1'b0;
            p0_s2_from_snoop      <= 1'b0;
            p0_s2_snoop_target    <= '0;
        end else if (p0_s2_READY) begin
            p0_s2_VALID <= p0_s1_grant_VALID;
            if (p0_s1_grant_VALID) begin
                p0_s2_req             <= p0_s1_grant_req;
                p0_s2_winner          <= p0_s1_grant_winner;
                p0_s2_index           <= p0_s1_index;
                p0_s2_grant           <= p0_s1_grant_msg;
                p0_s2_memory_required <= p0_s1_grant_memory_required;
                p0_s2_from_snoop      <= p0_s1_grant_from_snoop;
                p0_s2_snoop_target    <= p0_s1_grant_snoop_target;
            end
        end
    end
    /*  ---s2 */

    /* s4: register and send snoop request---  */
    coh_req_t                      p0_s3_req;
    logic [CACHE_ID_W-1:0]         p0_s3_winner;
    coh_snp_e                      p0_s3_msg;
    coh_rsp_e                      p0_s3_grant;
    logic                          p0_s3_memory_required;

    assign p0_s2_READY = !p0_s2_VALID ||
                         (!p0_s5_VALID && i_cache_rsp_READY[p0_s2_winner]);
                         
    for (genvar i = 0; i < NUM_CACHES; i++) begin : g_cache_snp_output
        always_comb begin
            o_cache_snp[i]              = '0;
            o_cache_snp[i].addr         = get_cache_line_addr(p0_s3_req.addr);
            o_cache_snp[i].requester_id = p0_s3_req.src_id;
            o_cache_snp[i].dst_id       = p0_s3_winner;
            o_cache_snp[i].trans_id     = p0_s3_req.trans_id;
            o_cache_snp[i].msg          = p0_s3_msg;
            o_cache_snp_VALID[i]        = p0_s3_VALID &&
                                          (p0_s3_winner == CACHE_ID_W'(i));
`ifdef KILL_STALLED_OUTPUT_STABILITY
            if (o_cache_snp_VALID[i] && !i_cache_snp_READY[i]) begin
                o_cache_snp[i].trans_id = p0_s3_req.trans_id + r_kill_stability_counter;
            end
`endif
            if (w_lock_snp_VALID && (w_lock_snp_target == CACHE_ID_W'(i))) begin
                o_cache_snp[i].addr         = w_lock_snp_addr;
                o_cache_snp[i].requester_id = r_owner;
                o_cache_snp[i].dst_id       = w_lock_snp_target;
                o_cache_snp[i].trans_id     = r_bus_req.id;
                o_cache_snp[i].msg          = SNP_INV;
                o_cache_snp_VALID[i]        = 1'b1;
            end
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s3_VALID           <= 1'b0;
            p0_s3_req             <= '0;
            p0_s3_winner          <= '0;
            p0_s3_msg             <= SNP_INV;
            p0_s3_grant           <= GRANT_S;
            p0_s3_memory_required <= 1'b0;
        end else if (p0_s3_READY) begin
            p0_s3_VALID <= p0_s1_VALID && p0_s1_needs_snoop;
            if (p0_s1_VALID && p0_s1_needs_snoop) begin
                p0_s3_req             <= p0_s1_req;
                p0_s3_winner          <= p0_s1_snoop_target;
                p0_s3_msg             <= p0_s1_snoop_msg;
                p0_s3_grant           <= p0_s1_grant;
                p0_s3_memory_required <= (p0_s1_snoop_msg == SNP_INV) &&
                                         !p0_s1_entry.sharers[p0_s1_winner];
            end
        end
    end
    /*  ---s4 */

    /* s5: wait for snoop acknowledgement---  */
    coh_req_t                      p0_s4_req;
    logic [CACHE_ID_W-1:0]         p0_s4_winner;
    coh_rsp_e                      p0_s4_grant;
    logic                          p0_s4_memory_required;
    logic                          p0_s4_complete;
    logic                          p0_s4_ack_match;

    assign p0_s3_READY = !p0_s3_VALID ||
                         (i_cache_snp_READY[p0_s3_winner] && p0_s4_READY);
    
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s4_VALID           <= 1'b0;
            p0_s4_req             <= '0;
            p0_s4_winner          <= '0;
            p0_s4_grant           <= GRANT_S;
            p0_s4_memory_required <= 1'b0;
        end else if (p0_s4_READY) begin
            p0_s4_VALID <= p0_s3_VALID && i_cache_snp_READY[p0_s3_winner];
            if (p0_s3_VALID && i_cache_snp_READY[p0_s3_winner]) begin
                p0_s4_req             <= p0_s3_req;
                p0_s4_winner          <= p0_s3_winner;
                p0_s4_grant           <= p0_s3_grant;
                p0_s4_memory_required <= p0_s3_memory_required;
            end
        end
    end

    /* s6: send grant after snoop completion---  */
    coh_req_t                      p0_s5_req;
    coh_rsp_e                      p0_s5_grant;
    logic                          p0_s5_memory_required;
`ifndef DUMB_COMPILER
    logic [CACHE_ID_W-1:0]         p0_s5_winner;
`endif

    assign p0_s4_READY     = !p0_s4_VALID || (p0_s4_complete && p0_s5_READY);
    assign p0_s4_ack_match = (i_cache_ack[p0_s4_winner].src_id == p0_s4_winner) &&
                             (i_cache_ack[p0_s4_winner].requester_id == p0_s4_req.src_id) &&
                             (i_cache_ack[p0_s4_winner].trans_id == p0_s4_req.trans_id) &&
                             (get_cache_line_addr(i_cache_ack[p0_s4_winner].addr) ==
                              get_cache_line_addr(p0_s4_req.addr));
`ifndef KILL_SNOOP_ACK_MATCH
    assign p0_s4_complete  = p0_s4_VALID && i_cache_ack_VALID[p0_s4_winner] &&
                             p0_s4_ack_match;
`else
    assign p0_s4_complete  = p0_s4_VALID && i_cache_ack_VALID[p0_s4_winner];
`endif

    always_comb begin
        o_cache_ack_READY = '{default: 1'b0};

        for (int i = 0; i < NUM_CACHES; i++) begin
            o_cache_ack_READY[i] = p0_s4_VALID && p0_s5_READY &&
                                   (p0_s4_winner == CACHE_ID_W'(i));
            if ((s_this inside {BUS_LOCK_SNOOP_WAIT_LO, BUS_LOCK_SNOOP_WAIT_HI}) &&
                (w_lock_snp_target == CACHE_ID_W'(i)))
                o_cache_ack_READY[i] = 1'b1;
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s5_VALID           <= 1'b0;
            p0_s5_req             <= '0;
            p0_s5_winner          <= '0;
            p0_s5_grant           <= GRANT_S;
            p0_s5_memory_required <= 1'b0;
        end else if (p0_s5_READY) begin
            p0_s5_VALID <= p0_s4_complete;
            if (p0_s4_complete) begin
                p0_s5_req             <= p0_s4_req;
                p0_s5_winner          <= p0_s4_winner;
                p0_s5_grant           <= p0_s4_grant;
                p0_s5_memory_required <= p0_s4_memory_required;
            end
        end
    end
    /*  ---s6 */
    assign p0_s5_READY = !p0_s5_VALID || i_cache_rsp_READY[p0_s5_req.src_id];
    /*  ---p0 */






    /* shared cache response arbitration---  */
    for (genvar i = 0; i < NUM_CACHES; i++) begin : g_cache_rsp_output
        always_comb begin
            o_cache_rsp[i]                 = '0;
            o_cache_rsp[i].addr            = get_cache_line_addr(p0_s2_req.addr);
            o_cache_rsp[i].dst_id          = p0_s2_winner;
            o_cache_rsp[i].trans_id        = p0_s2_req.trans_id;
            o_cache_rsp[i].memory_required = p0_s2_memory_required;
            o_cache_rsp[i].msg             = p0_s2_grant;
            o_cache_rsp_VALID[i]           = p0_s2_VALID &&
                                            (p0_s2_winner == CACHE_ID_W'(i));
`ifdef KILL_STALLED_OUTPUT_STABILITY
            if (o_cache_rsp_VALID[i] && !i_cache_rsp_READY[i]) begin
                o_cache_rsp[i].trans_id = p0_s2_req.trans_id + r_kill_stability_counter;
            end
`endif

            if (p0_s5_VALID) begin
                o_cache_rsp[i].addr            = get_cache_line_addr(p0_s5_req.addr);
                o_cache_rsp[i].dst_id          = p0_s5_req.src_id;
                o_cache_rsp[i].trans_id        = p0_s5_req.trans_id;
                o_cache_rsp[i].memory_required = p0_s5_memory_required;
                o_cache_rsp[i].msg             = p0_s5_grant;
                o_cache_rsp_VALID[i]           = p0_s5_req.src_id == CACHE_ID_W'(i);
            end
`ifdef KILL_REQUEST_RESPONSE_PROGRESS
            o_cache_rsp_VALID[i] = 1'b0;
`endif
        end
    end
    /*  ---shared cache response arbitration */






    /* shared directory update---  */
    always_ff @(posedge i_clk) begin
        automatic dir_addr_t update_addr;
        if (!i_rst_n) begin
            for (int i = 0; i < NUM_DIR_ENTRIES; i++) begin
                r_dir[i] <= '0;
            end
        end else if ((s_this inside {BUS_LOCK_SNOOP_WAIT_LO, BUS_LOCK_SNOOP_WAIT_HI}) &&
                     i_cache_ack_VALID[w_lock_snp_target] && w_lock_ack_match) begin
            update_addr = decode_dir_addr(w_lock_snp_addr);
            r_dir[update_addr.index].owner_valid                <= 1'b0;
            r_dir[update_addr.index].sharers[w_lock_snp_target] <= 1'b0;
        end else if (p0_s5_VALID && i_cache_rsp_READY[p0_s5_req.src_id]) begin
            update_addr = decode_dir_addr(p0_s5_req.addr);
            r_dir[update_addr.index].valid <= 1'b1;
            r_dir[update_addr.index].tag   <= update_addr.tag;
            if (p0_s5_req.msg == GETM) begin
`ifndef KILL_EXCLUSIVE_OWNER
                r_dir[update_addr.index].sharers     <= '0;
`endif
                r_dir[update_addr.index].owner_valid <= 1'b1;
                r_dir[update_addr.index].owner       <= p0_s5_req.src_id;
            end else begin
                r_dir[update_addr.index].owner_valid <= 1'b0;
                r_dir[update_addr.index].sharers[p0_s5_req.src_id] <= 1'b1;
                r_dir[update_addr.index].sharers[p0_s5_winner]    <= 1'b1;
            end
        end else if (p0_s2_VALID && i_cache_rsp_READY[p0_s2_winner]) begin
            update_addr = decode_dir_addr(p0_s2_req.addr);
            r_dir[update_addr.index].valid <= 1'b1;
            r_dir[update_addr.index].tag   <= update_addr.tag;
            if ((p0_s2_req.msg == GETM) || (p0_s2_grant == GRANT_E)) begin
`ifndef KILL_EXCLUSIVE_OWNER
                r_dir[update_addr.index].sharers     <= '0;
`endif
                r_dir[update_addr.index].owner_valid <= 1'b1;
                r_dir[update_addr.index].owner       <= p0_s2_winner;
            end else begin
                r_dir[update_addr.index].owner_valid           <= 1'b0;
                r_dir[update_addr.index].sharers[p0_s2_winner] <= 1'b1;
                if (p0_s2_from_snoop) begin
                    r_dir[update_addr.index].sharers[p0_s2_snoop_target] <= 1'b1;
                end
            end
        end else if (p0_s1_VALID && (p0_s1_req.msg inside {PUTS, PUTM})) begin
            r_dir[p0_s1_addr.index].sharers[p0_s1_winner] <= 1'b0;
            if ((p0_s1_req.msg == PUTM) ||
                (p0_s1_entry.owner_valid && (p0_s1_entry.owner == p0_s1_winner))) begin
                r_dir[p0_s1_addr.index].owner_valid <= 1'b0;
            end
        end
    end
    /*  ---shared directory update */

`else
    assign p0_s1_READY = 1'b1;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            p0_s2_VALID  <= 1'b0;
            p0_s2_winner <= CACHE_ID_W'(NUM_CACHES-1);
            p0_s2_index  <= '0;
        end else begin
            p0_s2_VALID <= p0_s1_VALID;
            if (p0_s1_VALID) begin
                p0_s2_winner <= p0_s1_winner;
                p0_s2_index  <= p0_s1_index;
            end
        end
    end

    always_comb begin
        o_cache_rsp       = '{default: coh_rsp_t'('0)};
        o_cache_rsp_VALID = '{default: 1'b0};
        o_cache_snp       = '{default: coh_snp_t'('0)};
        o_cache_snp_VALID = '{default: 1'b0};
        o_cache_ack_READY = '{default: 1'b0};
    end
`endif






`ifndef FORMAL_FAIR
    /*  bus lock serializer---  */
    logic [PORT_W-1:0] r_last_winner;
    logic [PORT_W-1:0] w_this_winner;
    logic              w_winner_VALID;

    always_comb begin
        w_this_winner  = '0;
        w_winner_VALID = 1'b0;

        for (int offset = 1; offset <= NUM_CACHES; offset++) begin
            if (!w_winner_VALID && i_lock_req_VALID[(r_last_winner + offset) % NUM_CACHES]) begin

                w_this_winner  = PORT_W'((r_last_winner + offset) % NUM_CACHES);
                w_winner_VALID = 1'b1;
            end
        end
        w_winner_VALID &= !p0_s1_VALID && !p0_s2_VALID && !p0_s3_VALID && !p0_s4_VALID && !p0_s5_VALID;
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            r_last_winner <= PORT_W'(NUM_CACHES-1);
        end else if (w_winner_VALID && o_lock_req_READY[w_this_winner]) begin
            r_last_winner <= w_this_winner;
        end
    end

    logic              [CACHE_LINE_W-1:0]   w_lines      [1:0];
    logic              [CACHE_LINE_W-1:0]   r_lines      [1:0];
    bus_lock_type_e                         w_lock_type;
    bus_lock_type_e                         r_lock_type;
    cpu_req_t                               w_bus_req;
    logic              [PORT_W-1:0]         w_owner;
    logic              [XLEN-1:0]           w_old_value;
    logic              [XLEN-1:0]           r_old_value;
    logic              [XLEN-1:0]           w_new_value;
    logic              [XLEN-1:0]           r_new_value;

    assign o_lock_mem_req_VALID = s_this inside {
        BUS_LOCK_READ_REQ_LO,
        BUS_LOCK_READ_REQ_HI,
        BUS_LOCK_WRITE_LO,
        BUS_LOCK_WRITE_HI
    };

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            s_this      <= BUS_LOCK_IDLE;
            r_lock_type <= BUS_LOCK_ILLEGAL;
            r_bus_req   <= '0;
            r_owner     <= '0;
            r_lines[0]  <= '0;
            r_lines[1]  <= '0;
            r_old_value <= '0;
            r_new_value <= '0;
        end else begin
            s_this      <= s_next;
            r_lock_type <= w_lock_type;
            r_bus_req   <= w_bus_req;
            r_owner     <= w_owner;
            r_lines[0]  <= w_lines[0];
            r_lines[1]  <= w_lines[1];
            r_old_value <= w_old_value;
            r_new_value <= w_new_value;
        end
    end

    always_comb begin
        automatic bus_lock_data_t data;
        automatic dir_addr_t      lock_dir_addr;
        automatic dir_entry_t     lock_dir_entry;
        automatic logic           lock_target_found;

        s_next           = s_this;
        w_lock_type      = r_lock_type;
        w_bus_req        = r_bus_req;
        w_owner          = r_owner;
        w_lines[0]       = r_lines[0];
        w_lines[1]       = r_lines[1];
        w_old_value      = r_old_value;
        w_new_value      = r_new_value;

        o_lock_req_READY = '{default: 1'b0};
        o_lock_rsp       = '{default: '0};
        o_lock_rsp_VALID = '{default: 1'b0};

        data = get_bus_lock_data(r_bus_req, r_lines[0], r_lines[1], r_new_value);

        for (int i = 0; i < NUM_CACHES; i++) begin
            o_lock_req_READY[i] = (s_this == BUS_LOCK_IDLE) && !w_winner_VALID;
            o_lock_rsp[i]       = '0;
            o_lock_rsp[i].id    = r_bus_req.id;
            o_lock_rsp[i].rdata = r_old_value;
            o_lock_rsp[i].atomic_success = cpu_op_is_atomic(r_bus_req.op);
            o_lock_rsp_VALID[i] = (s_this == BUS_LOCK_RESP) && (r_owner == i[PORT_W-1:0]);
        end

        o_lock_mem_req          = '0;
        o_lock_mem_req.trans_id = r_bus_req.id;
        o_lock_mem_rsp_READY    = 1'b0;
        w_lock_snp_VALID        = 1'b0;
        w_lock_snp_addr         = get_cache_line_addr(r_bus_req.addr);

        if (s_this inside {BUS_LOCK_SNOOP_REQ_HI, BUS_LOCK_SNOOP_WAIT_HI})
            w_lock_snp_addr += CACHE_LINE_W/8;

        lock_dir_addr       = decode_dir_addr(w_lock_snp_addr);
        lock_dir_entry      = r_dir[lock_dir_addr.index];
        w_lock_snp_target   = lock_dir_entry.owner;
        lock_target_found   = dir_entry_is_hit(lock_dir_entry, lock_dir_addr) &&
                              lock_dir_entry.owner_valid;

        for (int i = 0; i < NUM_CACHES; i++) begin
            if (!lock_target_found && dir_entry_is_hit(lock_dir_entry, lock_dir_addr) &&
                lock_dir_entry.sharers[i]) begin
                w_lock_snp_target = CACHE_ID_W'(i);
                lock_target_found = 1'b1;
            end
        end

        w_lock_ack_match = (i_cache_ack[w_lock_snp_target].src_id == w_lock_snp_target) &&
                           (i_cache_ack[w_lock_snp_target].requester_id == r_owner) &&
                           (i_cache_ack[w_lock_snp_target].trans_id == r_bus_req.id) &&
                           (get_cache_line_addr(i_cache_ack[w_lock_snp_target].addr) ==
                            w_lock_snp_addr);

        case(s_this)
            BUS_LOCK_IDLE: begin
                if (w_winner_VALID) begin
                    o_lock_req_READY[w_this_winner] = 1'b1;

                    if (i_lock_req_VALID[w_this_winner]) begin
                        w_bus_req   = i_lock_req[w_this_winner];
                        w_owner     = w_this_winner;
                        w_lock_type = get_bus_lock_type(
                                        i_lock_req[w_this_winner].op,
                                        i_lock_req[w_this_winner].mem_type,
                                        i_lock_req[w_this_winner].addr,
                                        i_lock_req[w_this_winner].size
                                    );
                    
                        if (w_lock_type != BUS_LOCK_ILLEGAL) begin
                            s_next = (w_lock_type == BUS_LOCK_MISALIGNED_CACHEABLE) ?
                                     BUS_LOCK_SNOOP_REQ_LO : BUS_LOCK_READ_REQ_LO;
                        end
                    end
                end
            end

            BUS_LOCK_SNOOP_REQ_LO: begin
                w_lock_snp_VALID = lock_target_found;
                if (!lock_target_found)
                    s_next = data.cross_line ? BUS_LOCK_SNOOP_REQ_HI : BUS_LOCK_READ_REQ_LO;
                else if (i_cache_snp_READY[w_lock_snp_target])
                    s_next = BUS_LOCK_SNOOP_WAIT_LO;
            end

            BUS_LOCK_SNOOP_WAIT_LO: begin
                if (i_cache_ack_VALID[w_lock_snp_target] && w_lock_ack_match)
                    s_next = BUS_LOCK_SNOOP_REQ_LO;
            end

            BUS_LOCK_SNOOP_REQ_HI: begin
                w_lock_snp_VALID = lock_target_found;
                if (!lock_target_found)
                    s_next = BUS_LOCK_READ_REQ_LO;
                else if (i_cache_snp_READY[w_lock_snp_target])
                    s_next = BUS_LOCK_SNOOP_WAIT_HI;
            end

            BUS_LOCK_SNOOP_WAIT_HI: begin
                if (i_cache_ack_VALID[w_lock_snp_target] && w_lock_ack_match)
                    s_next = BUS_LOCK_SNOOP_REQ_HI;
            end

            BUS_LOCK_READ_REQ_LO: begin
                o_lock_mem_req.addr  = get_cache_line_addr(r_bus_req.addr);

                if (i_lock_mem_req_READY) begin
                    s_next = BUS_LOCK_READ_RESP_LO;
                end
            end

            BUS_LOCK_READ_RESP_LO: begin
                o_lock_mem_rsp_READY = 1'b1;

                if (i_lock_mem_rsp_VALID) begin
                    w_lines[0] = i_lock_mem_rsp.rdata;

                    if ((r_lock_type != BUS_LOCK_ALIGNED_UNCACHEABLE) && data.cross_line) begin
                        s_next = BUS_LOCK_READ_REQ_HI;

                    end else begin
                        w_old_value = i_lock_mem_rsp.rdata >> data.shift;
                        w_new_value = execute_atomic(
                                        r_bus_req.op,
                                        i_lock_mem_rsp.rdata >> data.shift,
                                        r_bus_req.wdata, 
                                        r_bus_req.compare_data
                                    );
                        s_next      = BUS_LOCK_WRITE_LO;
                    end
                end
            end

            BUS_LOCK_READ_REQ_HI: begin
                o_lock_mem_req.addr  = get_cache_line_addr(r_bus_req.addr) + (CACHE_LINE_W/8);

                if (i_lock_mem_req_READY) begin
                    s_next = BUS_LOCK_READ_RESP_HI;
                end
            end

            BUS_LOCK_READ_RESP_HI: begin
                o_lock_mem_rsp_READY = 1'b1;

                if (i_lock_mem_rsp_VALID) begin
                    w_lines[1]  = i_lock_mem_rsp.rdata;
                    w_old_value = ({i_lock_mem_rsp.rdata, r_lines[0]} >> data.shift);
                    w_new_value = execute_atomic(
                                    r_bus_req.op,
                                    ({i_lock_mem_rsp.rdata, r_lines[0]} >> data.shift),
                                    r_bus_req.wdata, 
                                    r_bus_req.compare_data
                                );
                    s_next      = BUS_LOCK_WRITE_LO;
                end
            end

            BUS_LOCK_WRITE_LO: begin
                o_lock_mem_req.wen   = 1'b1;
                o_lock_mem_req.addr  = get_cache_line_addr(r_bus_req.addr);
                o_lock_mem_req.wdata = data.lines[0 +: CACHE_LINE_W];
                o_lock_mem_req.wstrb = data.wstrb[0 +: CACHE_LINE_W/8];

                if (i_lock_mem_req_READY) begin
                    s_next = ((r_lock_type != BUS_LOCK_ALIGNED_UNCACHEABLE) &&
                              data.cross_line) ? BUS_LOCK_WRITE_HI : BUS_LOCK_RESP;
                end
            end

            BUS_LOCK_WRITE_HI: begin
                o_lock_mem_req.wen   = 1'b1;
                o_lock_mem_req.addr  = get_cache_line_addr(r_bus_req.addr) + (CACHE_LINE_W/8);
                o_lock_mem_req.wdata = data.lines[CACHE_LINE_W +: CACHE_LINE_W];
                o_lock_mem_req.wstrb = data.wstrb[CACHE_LINE_W/8 +: CACHE_LINE_W/8];

                if (i_lock_mem_req_READY) begin
                    s_next = BUS_LOCK_RESP;
                end
            end

            BUS_LOCK_RESP: begin
                if (o_lock_rsp_VALID[r_owner] && i_lock_rsp_READY[r_owner]) begin
                    s_next = BUS_LOCK_IDLE;
                end
            end

            default: s_next = BUS_LOCK_IDLE;
        endcase
    end
    /* ---serializer */
`else
`ifdef FORMAL_TYPE_FAIR
    logic r_formal_lock_busy;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            r_formal_lock_busy <= 1'b0;
        end else if (r_formal_lock_busy && i_lock_rsp_READY[1]) begin
            r_formal_lock_busy <= 1'b0;
        end else if (i_lock_req_VALID[1] && o_lock_req_READY[1]) begin
            r_formal_lock_busy <= 1'b1;
        end
    end

    always_comb begin
        s_this                = BUS_LOCK_IDLE;
        s_next                = BUS_LOCK_IDLE;
        r_bus_req             = '0;
        r_owner               = '0;
        w_lock_snp_VALID      = 1'b0;
        w_lock_snp_target     = '0;
        w_lock_snp_addr       = '0;
        w_lock_ack_match      = 1'b0;
        o_lock_req_READY      = '{default: 1'b0};
        o_lock_req_READY[1]   = !r_formal_lock_busy &&
                                !p0_s1_VALID && !p0_s2_VALID;
        o_lock_rsp            = '{default: '0};
        o_lock_rsp_VALID      = '{default: 1'b0};
        o_lock_rsp_VALID[1]   = r_formal_lock_busy;
        o_lock_mem_req        = '0;
        o_lock_mem_req_VALID  = 1'b0;
        o_lock_mem_rsp_READY  = 1'b0;
    end
`else
    always_comb begin
        s_this                = BUS_LOCK_IDLE;
        s_next                = BUS_LOCK_IDLE;
        r_bus_req             = '0;
        r_owner               = '0;
        w_lock_snp_VALID      = 1'b0;
        w_lock_snp_target     = '0;
        w_lock_snp_addr       = '0;
        w_lock_ack_match      = 1'b0;
        o_lock_req_READY      = '{default: 1'b0};
        o_lock_rsp            = '{default: '0};
        o_lock_rsp_VALID      = '{default: 1'b0};
        o_lock_mem_req        = '0;
        o_lock_mem_req_VALID  = 1'b0;
        o_lock_mem_rsp_READY  = 1'b0;
    end
`endif
`endif

`else
    /* Property2 COI: keep only the CPU line reservation scoreboard. */
    assign p0_s1_VALID  = 1'b0;
    assign p0_s1_READY  = 1'b0;
    assign p0_s1_index  = '0;
    assign p0_s2_VALID  = 1'b0;
    assign p0_s2_winner = '0;
    assign p0_s2_index  = '0;
    assign o_lock_req_READY      = '{default: 1'b0};
    assign o_lock_rsp            = '{default: cpu_rsp_t'('0)};
    assign o_lock_rsp_VALID      = '{default: 1'b0};
    assign o_cache_rsp           = '{default: coh_rsp_t'('0)};
    assign o_cache_rsp_VALID     = '{default: 1'b0};
    assign o_cache_snp           = '{default: coh_snp_t'('0)};
    assign o_cache_snp_VALID     = '{default: 1'b0};
    assign o_cache_ack_READY     = '{default: 1'b0};
    assign o_lock_mem_req        = '0;
    assign o_lock_mem_req_VALID  = 1'b0;
    assign o_lock_mem_rsp_READY  = 1'b0;
`endif


`ifdef FORMAL_EXCLUSIVE_OWNER
    /* Property-3 COI: retain only the ownership/sharer state transition. */
    always_comb begin
        automatic logic                   next_owner_valid;
        automatic logic [NUM_CACHES-1:0] next_sharers;

        next_owner_valid = i_formal_owner_valid;
        next_sharers     = i_formal_sharers;
        for (int client = 0; client < NUM_CACHES; client++) begin
            if (i_cache_req_VALID[client]) begin
                if (i_cache_req[client].msg == GETM) begin
`ifndef KILL_EXCLUSIVE_OWNER
                    next_sharers = '0;
`endif
`ifdef KILL_EXCLUSIVE_OWNER_RESTORE_SHARERS
                    next_sharers = '0;
                    next_sharers[client] = 1'b1;
`endif

                    next_owner_valid = 1'b1;
                end else if (i_cache_req[client].msg == GETS) begin
                    next_owner_valid     = 1'b0;
                    next_sharers[client] = 1'b1;
                end
            end
        end
        o_formal_exclusive_violation = next_owner_valid && (next_sharers != '0);
    end
`endif

`ifndef FORMAL_FAIR
    /* shared scoreboard---  */
    line_reservation_t             r_scoreboard [NUM_SCOREBOARD_ENTRIES-1:0];
    logic [LINE_RESERVATION_W-1:0] w_free_slot  [NUM_CACHES-1:0];
    logic                          w_slot_found [NUM_CACHES-1:0];
    logic                          w_line_busy  [NUM_CACHES-1:0];

`ifdef FORMAL_SCOREBOARD
    assign o_formal_slot_0_valid    = r_scoreboard[0].valid;
    assign o_formal_slot_1_valid    = r_scoreboard[1].valid;
    assign o_formal_slot_addr_equal = r_scoreboard[0].addr == r_scoreboard[1].addr;
`endif

`ifndef FORMAL_SCOREBOARD
    always_comb begin
        automatic logic slot_claimed [NUM_SCOREBOARD_ENTRIES-1:0];

        w_free_slot     = '{default: '0};
        w_slot_found    = '{default: 1'b0};
        w_line_busy     = '{default: 1'b0};
        slot_claimed    = '{default: 1'b0};
        o_cpu_req_allow = '{default: 1'b1};

        for (int client = 0; client < NUM_CACHES; client++) begin
            for (int slot = 0; slot < NUM_SCOREBOARD_ENTRIES; slot++) begin
                w_line_busy[client] |= r_scoreboard[slot].valid &&
                (r_scoreboard[slot].addr == get_cache_line_addr(i_cpu_req[client].addr));

                if (!w_slot_found[client] && !r_scoreboard[slot].valid && !slot_claimed[slot]) begin

                    w_free_slot[client]  = LINE_RESERVATION_W'(slot);
                    w_slot_found[client] = 1'b1;
                    slot_claimed[slot]   = 1'b1;
                end
            end

            for (int earlier = 0; earlier < client; earlier++) begin
                w_line_busy[client] |= i_cpu_req_VALID[earlier] &&
                    (get_cache_line_addr(i_cpu_req[earlier].addr) ==
                     get_cache_line_addr(i_cpu_req[client].addr));
            end

`ifndef KILL_SCOREBOARD_SAME_LINE
            o_cpu_req_allow[client] = !i_cpu_req_VALID[client] || (!w_line_busy[client] && w_slot_found[client]);
`else
            o_cpu_req_allow[client] = !i_cpu_req_VALID[client] || w_slot_found[client];
`endif
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            for (int slot = 0; slot < NUM_SCOREBOARD_ENTRIES; slot++) begin
                r_scoreboard[slot] <= '0;
            end
        end else begin
            for (int slot = 0; slot < NUM_SCOREBOARD_ENTRIES; slot++) begin
                for (int client = 0; client < NUM_CACHES; client++) begin
                    if (r_scoreboard[slot].valid &&
                        i_cpu_rsp_VALID[client] && i_cpu_rsp_READY[client] &&
                        (r_scoreboard[slot].owner == CACHE_ID_W'(client)) &&
                        (r_scoreboard[slot].id == i_cpu_rsp[client].id)) begin
                        r_scoreboard[slot].valid <= 1'b0;
                    end
                end
            end

            for (int client = 0; client < NUM_CACHES; client++) begin
                if (i_cpu_req_VALID[client] && i_cpu_req_READY[client] &&
                    o_cpu_req_allow[client]) begin
                    r_scoreboard[w_free_slot[client]].valid <= 1'b1;
                    r_scoreboard[w_free_slot[client]].addr  <=
                    get_cache_line_addr(i_cpu_req[client].addr);
                    r_scoreboard[w_free_slot[client]].owner <= CACHE_ID_W'(client);
                    r_scoreboard[w_free_slot[client]].id    <= i_cpu_req[client].id;
                end
            end
        end
    end

`else
    always_comb begin
        w_free_slot[0]  = !r_scoreboard[0].valid ? '0 : LINE_RESERVATION_W'(1);
        w_slot_found[0] = !r_scoreboard[0].valid || !r_scoreboard[1].valid;
        w_free_slot[1]  = LINE_RESERVATION_W'(1);
        w_slot_found[1] = !r_scoreboard[0].valid && !r_scoreboard[1].valid;

        w_line_busy[0] = (r_scoreboard[0].valid &&
            (r_scoreboard[0].addr == get_cache_line_addr(i_cpu_req[0].addr))) ||
            (r_scoreboard[1].valid &&
            (r_scoreboard[1].addr == get_cache_line_addr(i_cpu_req[0].addr)));
        w_line_busy[1] = (r_scoreboard[0].valid &&
            (r_scoreboard[0].addr == get_cache_line_addr(i_cpu_req[1].addr))) ||
            (r_scoreboard[1].valid &&
            (r_scoreboard[1].addr == get_cache_line_addr(i_cpu_req[1].addr))) ||
            (i_cpu_req_VALID[0] &&
            (get_cache_line_addr(i_cpu_req[0].addr) ==
             get_cache_line_addr(i_cpu_req[1].addr)));

`ifndef KILL_SCOREBOARD_SAME_LINE
        o_cpu_req_allow[0] = !i_cpu_req_VALID[0] ||
                             (!w_line_busy[0] && w_slot_found[0]);
        o_cpu_req_allow[1] = !i_cpu_req_VALID[1] ||
                             (!w_line_busy[1] && w_slot_found[1]);
`else
        o_cpu_req_allow[0] = !i_cpu_req_VALID[0] || w_slot_found[0];
        o_cpu_req_allow[1] = !i_cpu_req_VALID[1] || w_slot_found[1];
`endif
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            r_scoreboard[0] <= '0;
            r_scoreboard[1] <= '0;
        end else begin
            if (i_cpu_req_VALID[0] && i_cpu_req_READY[0] && o_cpu_req_allow[0]) begin
                if (w_free_slot[0] == '0) begin
                    r_scoreboard[0].valid <= 1'b1;
                    r_scoreboard[0].addr  <= get_cache_line_addr(i_cpu_req[0].addr);
                    r_scoreboard[0].owner <= CACHE_ID_W'(0);
                    r_scoreboard[0].id    <= i_cpu_req[0].id;
                end else begin
                    r_scoreboard[1].valid <= 1'b1;
                    r_scoreboard[1].addr  <= get_cache_line_addr(i_cpu_req[0].addr);
                    r_scoreboard[1].owner <= CACHE_ID_W'(0);
                    r_scoreboard[1].id    <= i_cpu_req[0].id;
                end
            end
            if (i_cpu_req_VALID[1] && i_cpu_req_READY[1] && o_cpu_req_allow[1]) begin
                r_scoreboard[1].valid <= 1'b1;
                r_scoreboard[1].addr  <= get_cache_line_addr(i_cpu_req[1].addr);
                r_scoreboard[1].owner <= CACHE_ID_W'(1);
                r_scoreboard[1].id    <= i_cpu_req[1].id;
            end
        end
    end
`endif
    /*  ---shared scoreboard */
`else
    assign o_cpu_req_allow = '{default: 1'b1};
`endif

`else
    /* Property1 COI: only the combinational request arbiter */
    assign p0_s1_VALID  = 1'b0;
    assign p0_s1_READY  = 1'b1;
    assign p0_s1_index  = '0;
    assign p0_s2_VALID  = 1'b0;
    assign p0_s2_winner = '0;
    assign p0_s2_index  = '0;
    assign o_cpu_req_allow       = '{default: 1'b1};
    assign o_lock_req_READY      = '{default: 1'b0};
    assign o_lock_rsp            = '{default: cpu_rsp_t'('0)};
    assign o_lock_rsp_VALID      = '{default: 1'b0};
    assign o_cache_rsp           = '{default: coh_rsp_t'('0)};
    assign o_cache_rsp_VALID     = '{default: 1'b0};
    assign o_cache_snp           = '{default: coh_snp_t'('0)};
    assign o_cache_snp_VALID     = '{default: 1'b0};
    assign o_cache_ack_READY     = '{default: 1'b0};
    assign o_lock_mem_req        = '0;
    assign o_lock_mem_req_VALID  = 1'b0;
    assign o_lock_mem_rsp_READY  = 1'b0;
`endif

`ifdef FORMAL_ACK_MATCH
    logic r_formal_conflicting_grant;

    assign o_formal_ack_match =
        (i_cache_ack[i_formal_snoop_target].src_id == i_formal_snoop_target) &&
        (i_cache_ack[i_formal_snoop_target].requester_id == i_formal_snoop_req.src_id) &&
        (i_cache_ack[i_formal_snoop_target].trans_id == i_formal_snoop_req.trans_id) &&
        (get_cache_line_addr(i_cache_ack[i_formal_snoop_target].addr) ==
         get_cache_line_addr(i_formal_snoop_req.addr));
`ifndef KILL_SNOOP_ACK_MATCH
    assign o_formal_snoop_complete = i_formal_wait_valid &&
                                     i_cache_ack_VALID[i_formal_snoop_target] &&
                                     o_formal_ack_match;
`else
    assign o_formal_snoop_complete = i_formal_wait_valid &&
                                     i_cache_ack_VALID[i_formal_snoop_target];
`endif

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n)
            r_formal_conflicting_grant <= 1'b0;
        else
            r_formal_conflicting_grant <= o_formal_snoop_complete;
    end

    assign o_formal_conflicting_grant = r_formal_conflicting_grant;
`endif

`ifdef FORMAL_STABILITY
    always_comb begin
        o_formal_rsp_next_valid = i_formal_rsp_valid;
        o_formal_rsp_next       = i_formal_rsp;
        o_formal_snp_next_valid = i_formal_snp_valid;
        o_formal_snp_next       = i_formal_snp;
`ifdef KILL_STALLED_OUTPUT_STABILITY
        if (i_formal_rsp_valid && !i_formal_rsp_ready) begin
            o_formal_rsp_next.trans_id = i_formal_rsp.trans_id + 1'b1;
        end
        if (i_formal_snp_valid && !i_formal_snp_ready) begin
            o_formal_snp_next.trans_id = i_formal_snp.trans_id + 1'b1;
        end
`endif
`ifdef KILL_STALLED_OUTPUT_DROP_VALID
        if (i_formal_rsp_valid && !i_formal_rsp_ready)
            o_formal_rsp_next_valid = 1'b0;
        if (i_formal_snp_valid && !i_formal_snp_ready)
            o_formal_snp_next_valid = 1'b0;
`endif
    end
`endif

endmodule: directory
