`default_nettype none

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;
import directory_pkg::*;

module directory_muexc (
    input logic i_clk,
    input logic i_rst_n,
    input logic f_req_valid_0,
    input logic f_req_valid_1,
    output logic f_req_ready_0,
    output logic f_req_ready_1
);

    cpu_req_t  i_cpu_req               [NUM_CACHES-1:0];
    logic      i_cpu_req_VALID         [NUM_CACHES-1:0];
    logic      i_cpu_req_READY         [NUM_CACHES-1:0];
    logic      o_cpu_req_allow         [NUM_CACHES-1:0];
    cpu_rsp_t  i_cpu_rsp               [NUM_CACHES-1:0];
    logic      i_cpu_rsp_VALID         [NUM_CACHES-1:0];
    logic      i_cpu_rsp_READY         [NUM_CACHES-1:0];

    cpu_req_t  i_lock_req              [NUM_CACHES-1:0];
    logic      i_lock_req_VALID        [NUM_CACHES-1:0];
    logic      o_lock_req_READY        [NUM_CACHES-1:0];
    cpu_rsp_t  o_lock_rsp              [NUM_CACHES-1:0];
    logic      o_lock_rsp_VALID        [NUM_CACHES-1:0];
    logic      i_lock_rsp_READY        [NUM_CACHES-1:0];

    coh_req_t  i_cache_req             [NUM_CACHES-1:0];
    logic      i_cache_req_VALID       [NUM_CACHES-1:0];
    logic      o_cache_req_READY       [NUM_CACHES-1:0];
    coh_rsp_t  o_cache_rsp             [NUM_CACHES-1:0];
    logic      o_cache_rsp_VALID       [NUM_CACHES-1:0];
    logic      i_cache_rsp_READY       [NUM_CACHES-1:0];
    coh_snp_t  o_cache_snp             [NUM_CACHES-1:0];
    logic      o_cache_snp_VALID       [NUM_CACHES-1:0];
    logic      i_cache_snp_READY       [NUM_CACHES-1:0];
    coh_ack_t  i_cache_ack             [NUM_CACHES-1:0];
    logic      i_cache_ack_VALID       [NUM_CACHES-1:0];
    logic      o_cache_ack_READY       [NUM_CACHES-1:0];

    mem_req_t  o_lock_mem_req;
    logic      o_lock_mem_req_VALID;
    logic      i_lock_mem_req_READY;
    mem_rsp_t  i_lock_mem_rsp;
    logic      i_lock_mem_rsp_VALID;
    logic      o_lock_mem_rsp_READY;

    logic      accept_0;
    logic      accept_1;

    always_comb begin
`ifdef SYMBIYOSYS_COMPATIBLE
        i_cpu_req             = '{default: cpu_req_t'('0)};
`else
        i_cpu_req             = '{default: '0};
`endif
        i_cpu_req_VALID       = '{default: 1'b0};
        i_cpu_req_READY       = '{default: 1'b0};
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
        i_lock_mem_req_READY  = 1'b1;
        i_lock_mem_rsp        = '0;
        i_lock_mem_rsp_VALID  = 1'b0;
`ifdef SYMBIYOSYS_COMPATIBLE
        i_cache_req           = '{default: coh_req_t'('0)};
`else
        i_cache_req           = '{default: '0};
`endif
        i_cache_req[0].addr   = 64'h0000;
        i_cache_req[0].src_id = 0;
        i_cache_req[0].msg    = GETS;
        i_cache_req[1].addr   = 64'h0040;
        i_cache_req[1].src_id = 1;
        i_cache_req[1].msg    = GETS;
        i_cache_req_VALID[0]  = f_req_valid_0;
        i_cache_req_VALID[1]  = f_req_valid_1;
        i_cache_rsp_READY     = '{default: 1'b1};
        i_cache_snp_READY     = '{default: 1'b1};
`ifdef SYMBIYOSYS_COMPATIBLE
        i_cache_ack           = '{default: coh_ack_t'('0)};
`else
        i_cache_ack           = '{default: '0};
`endif
        i_cache_ack_VALID     = '{default: 1'b0};
    end

    directory #(
        .NUM_DIR_ENTRIES      (2)
    ) dut (
        .i_clk                (i_clk),
        .i_rst_n              (i_rst_n),
        .i_cpu_req            (i_cpu_req),
        .i_cpu_req_VALID      (i_cpu_req_VALID),
        .i_cpu_req_READY      (i_cpu_req_READY),
        .o_cpu_req_allow      (o_cpu_req_allow),
        .i_cpu_rsp            (i_cpu_rsp),
        .i_cpu_rsp_VALID      (i_cpu_rsp_VALID),
        .i_cpu_rsp_READY      (i_cpu_rsp_READY),
        .i_lock_req           (i_lock_req),
        .i_lock_req_VALID     (i_lock_req_VALID),
        .o_lock_req_READY     (o_lock_req_READY),
        .o_lock_rsp           (o_lock_rsp),
        .o_lock_rsp_VALID     (o_lock_rsp_VALID),
        .i_lock_rsp_READY     (i_lock_rsp_READY),
        .i_cache_req          (i_cache_req),
        .i_cache_req_VALID    (i_cache_req_VALID),
        .o_cache_req_READY    (o_cache_req_READY),
        .o_cache_rsp          (o_cache_rsp),
        .o_cache_rsp_VALID    (o_cache_rsp_VALID),
        .i_cache_rsp_READY    (i_cache_rsp_READY),
        .o_cache_snp          (o_cache_snp),
        .o_cache_snp_VALID    (o_cache_snp_VALID),
        .i_cache_snp_READY    (i_cache_snp_READY),
        .i_cache_ack          (i_cache_ack),
        .i_cache_ack_VALID    (i_cache_ack_VALID),
        .o_cache_ack_READY    (o_cache_ack_READY),
        .o_lock_mem_req       (o_lock_mem_req),
        .o_lock_mem_req_VALID (o_lock_mem_req_VALID),
        .i_lock_mem_req_READY (i_lock_mem_req_READY),
        .i_lock_mem_rsp       (i_lock_mem_rsp),
        .i_lock_mem_rsp_VALID (i_lock_mem_rsp_VALID),
        .o_lock_mem_rsp_READY (o_lock_mem_rsp_READY)
    );

    assign f_req_ready_0 = o_cache_req_READY[0];
    assign f_req_ready_1 = o_cache_req_READY[1];
    assign accept_0      = f_req_valid_0 && f_req_ready_0;
    assign accept_1      = f_req_valid_1 && f_req_ready_1;

    `include "directory_env_muexc.sv"
    `include "directory_muexc_properties.sv"
endmodule

`default_nettype wire
