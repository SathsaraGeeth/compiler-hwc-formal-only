`default_nettype none

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;
import directory_pkg::*;

module directory_ack_match (
    input logic i_clk,
    input logic i_rst_n,
    input logic f_wait_valid,
    input logic f_ack_valid,
    input logic [CACHE_ID_W-1:0] f_ack_src,
    input logic [CACHE_ID_W-1:0] f_ack_requester,
    input logic [TRANS_ID_W-1:0] f_ack_trans_id,
    input logic [PHY_ADDR_W-1:0] f_ack_addr,
    input logic [CACHE_ID_W-1:0] f_req_src,
    input logic [TRANS_ID_W-1:0] f_req_trans_id,
    input logic [PHY_ADDR_W-1:0] f_req_addr,
    output logic f_ack_match,
    output logic f_snoop_complete,
    output logic f_conflicting_grant
);
    cpu_req_t i_cpu_req [NUM_CACHES-1:0];
    logic i_cpu_req_VALID [NUM_CACHES-1:0];
    logic i_cpu_req_READY [NUM_CACHES-1:0];
    logic o_cpu_req_allow [NUM_CACHES-1:0];
    cpu_rsp_t i_cpu_rsp [NUM_CACHES-1:0];
    logic i_cpu_rsp_VALID [NUM_CACHES-1:0];
    logic i_cpu_rsp_READY [NUM_CACHES-1:0];
    cpu_req_t i_lock_req [NUM_CACHES-1:0];
    logic i_lock_req_VALID [NUM_CACHES-1:0];
    logic o_lock_req_READY [NUM_CACHES-1:0];
    cpu_rsp_t o_lock_rsp [NUM_CACHES-1:0];
    logic o_lock_rsp_VALID [NUM_CACHES-1:0];
    logic i_lock_rsp_READY [NUM_CACHES-1:0];
    coh_req_t i_cache_req [NUM_CACHES-1:0];
    logic i_cache_req_VALID [NUM_CACHES-1:0];
    logic o_cache_req_READY [NUM_CACHES-1:0];
    coh_rsp_t o_cache_rsp [NUM_CACHES-1:0];
    logic o_cache_rsp_VALID [NUM_CACHES-1:0];
    logic i_cache_rsp_READY [NUM_CACHES-1:0];
    coh_snp_t o_cache_snp [NUM_CACHES-1:0];
    logic o_cache_snp_VALID [NUM_CACHES-1:0];
    logic i_cache_snp_READY [NUM_CACHES-1:0];
    coh_ack_t i_cache_ack [NUM_CACHES-1:0];
    logic i_cache_ack_VALID [NUM_CACHES-1:0];
    logic o_cache_ack_READY [NUM_CACHES-1:0];
    mem_req_t o_lock_mem_req;
    logic o_lock_mem_req_VALID;
    logic i_lock_mem_req_READY;
    mem_rsp_t i_lock_mem_rsp;
    logic i_lock_mem_rsp_VALID;
    logic o_lock_mem_rsp_READY;
    coh_req_t formal_req;

    always_comb begin
`ifdef SYMBIYOSYS_COMPATIBLE
        i_cpu_req             = '{default: cpu_req_t'('0)};
`else
        i_cpu_req             = '{default: '0};
`endif
        i_cpu_req_VALID       = '{default: 1'b0};
        i_cpu_req_READY       = '{default: 1'b1};
        i_cpu_rsp             = '{default: '0};
        i_cpu_rsp_VALID       = '{default: 1'b0};
        i_cpu_rsp_READY       = '{default: 1'b1};
`ifdef SYMBIYOSYS_COMPATIBLE
        i_lock_req            = '{default: cpu_req_t'('0)};
`else
        i_lock_req            = '{default: '0};
`endif
        i_lock_req_VALID      = '{default: 1'b0};
        i_lock_rsp_READY      = '{default: 1'b1};
`ifdef SYMBIYOSYS_COMPATIBLE
        i_cache_req           = '{default: coh_req_t'('0)};
`else
        i_cache_req           = '{default: '0};
`endif
        i_cache_req_VALID     = '{default: 1'b0};
        i_cache_rsp_READY     = '{default: 1'b1};
        i_cache_snp_READY     = '{default: 1'b1};
`ifdef SYMBIYOSYS_COMPATIBLE
        i_cache_ack           = '{default: coh_ack_t'('0)};
`else
        i_cache_ack           = '{default: '0};
`endif
        i_cache_ack[0].src_id       = f_ack_src;
        i_cache_ack[0].requester_id = f_ack_requester;
        i_cache_ack[0].trans_id     = f_ack_trans_id;
        i_cache_ack[0].addr         = f_ack_addr;
        i_cache_ack_VALID     = '{default: 1'b0};
        i_cache_ack_VALID[0]  = f_ack_valid;
        i_lock_mem_req_READY  = 1'b1;
        i_lock_mem_rsp        = '0;
        i_lock_mem_rsp_VALID  = 1'b0;
        formal_req            = '0;
        formal_req.src_id     = f_req_src;
        formal_req.trans_id   = f_req_trans_id;
        formal_req.addr       = f_req_addr;
    end

    directory #(.NUM_DIR_ENTRIES(2)) dut (
        .i_clk, .i_rst_n,
        .i_cpu_req, .i_cpu_req_VALID, .i_cpu_req_READY, .o_cpu_req_allow,
        .i_cpu_rsp, .i_cpu_rsp_VALID, .i_cpu_rsp_READY,
        .i_lock_req, .i_lock_req_VALID, .o_lock_req_READY,
        .o_lock_rsp, .o_lock_rsp_VALID, .i_lock_rsp_READY,
        .i_cache_req, .i_cache_req_VALID, .o_cache_req_READY,
        .o_cache_rsp, .o_cache_rsp_VALID, .i_cache_rsp_READY,
        .o_cache_snp, .o_cache_snp_VALID, .i_cache_snp_READY,
        .i_cache_ack, .i_cache_ack_VALID, .o_cache_ack_READY,
        .o_lock_mem_req, .o_lock_mem_req_VALID, .i_lock_mem_req_READY,
        .i_lock_mem_rsp, .i_lock_mem_rsp_VALID, .o_lock_mem_rsp_READY,
        .i_formal_wait_valid(f_wait_valid),
        .i_formal_snoop_target(CACHE_ID_W'(0)),
        .i_formal_snoop_req(formal_req),
        .o_formal_ack_match(f_ack_match),
        .o_formal_snoop_complete(f_snoop_complete),
        .o_formal_conflicting_grant(f_conflicting_grant)
    );

    `include "directory_env_ack_match.sv"
`ifndef VERILATOR
    `include "directory_ack_match_properties.sv"
`endif
endmodule

`default_nettype wire
