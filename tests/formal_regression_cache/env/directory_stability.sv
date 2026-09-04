`default_nettype none

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;
import directory_pkg::*;

module directory_stability (
    input logic i_clk, input logic i_rst_n,
    input logic f_rsp_valid, input coh_rsp_t f_rsp, input logic f_rsp_ready,
    input logic f_snp_valid, input coh_snp_t f_snp, input logic f_snp_ready,
    output logic f_rsp_next_valid, output coh_rsp_t f_rsp_next,
    output logic f_snp_next_valid, output coh_snp_t f_snp_next
);
    cpu_req_t cpu_req [NUM_CACHES-1:0];
    cpu_rsp_t cpu_rsp [NUM_CACHES-1:0];
    cpu_req_t lock_req [NUM_CACHES-1:0];
    cpu_rsp_t lock_rsp [NUM_CACHES-1:0];
    coh_req_t cache_req [NUM_CACHES-1:0];
    coh_rsp_t cache_rsp [NUM_CACHES-1:0];
    coh_snp_t cache_snp [NUM_CACHES-1:0];
    coh_ack_t cache_ack [NUM_CACHES-1:0];
    logic cpu_req_valid [NUM_CACHES-1:0], cpu_req_ready [NUM_CACHES-1:0];
    logic cpu_req_allow [NUM_CACHES-1:0], cpu_rsp_valid [NUM_CACHES-1:0];
    logic cpu_rsp_ready [NUM_CACHES-1:0], lock_req_valid [NUM_CACHES-1:0];
    logic lock_req_ready [NUM_CACHES-1:0], lock_rsp_valid [NUM_CACHES-1:0];
    logic lock_rsp_ready [NUM_CACHES-1:0], cache_req_valid [NUM_CACHES-1:0];
    logic cache_req_ready [NUM_CACHES-1:0], cache_rsp_valid [NUM_CACHES-1:0];
    logic cache_rsp_ready [NUM_CACHES-1:0], cache_snp_valid [NUM_CACHES-1:0];
    logic cache_snp_ready [NUM_CACHES-1:0], cache_ack_valid [NUM_CACHES-1:0];
    logic cache_ack_ready [NUM_CACHES-1:0];
    mem_req_t lock_mem_req;
    mem_rsp_t lock_mem_rsp;
    logic lock_mem_req_valid, lock_mem_req_ready, lock_mem_rsp_valid, lock_mem_rsp_ready;

    always_comb begin
`ifdef SYMBIYOSYS_COMPATIBLE
        cpu_req = '{default: cpu_req_t'('0)};
`else
        cpu_req = '{default: '0};
`endif
        cpu_req_valid = '{default: 1'b0};
        cpu_req_ready = '{default: 1'b1}; cpu_rsp = '{default: '0};
        cpu_rsp_valid = '{default: 1'b0}; cpu_rsp_ready = '{default: 1'b1};
`ifdef SYMBIYOSYS_COMPATIBLE
        lock_req = '{default: cpu_req_t'('0)};
        cache_req = '{default: coh_req_t'('0)};
`else
        lock_req = '{default: '0};
        cache_req = '{default: '0};
`endif
        lock_req_valid = '{default: 1'b0};
        lock_rsp_ready = '{default: 1'b1};
        cache_req_valid = '{default: 1'b0}; cache_rsp_ready = '{default: 1'b1};
        cache_snp_ready = '{default: 1'b1};
`ifdef SYMBIYOSYS_COMPATIBLE
        cache_ack = '{default: coh_ack_t'('0)};
`else
        cache_ack = '{default: '0};
`endif
        cache_ack_valid = '{default: 1'b0}; lock_mem_req_ready = 1'b1;
        lock_mem_rsp = '0; lock_mem_rsp_valid = 1'b0;
    end

    directory #(.NUM_DIR_ENTRIES(2)) dut (
        .i_clk, .i_rst_n,
        .i_cpu_req(cpu_req), .i_cpu_req_VALID(cpu_req_valid),
        .i_cpu_req_READY(cpu_req_ready), .o_cpu_req_allow(cpu_req_allow),
        .i_cpu_rsp(cpu_rsp), .i_cpu_rsp_VALID(cpu_rsp_valid), .i_cpu_rsp_READY(cpu_rsp_ready),
        .i_lock_req(lock_req), .i_lock_req_VALID(lock_req_valid),
        .o_lock_req_READY(lock_req_ready), .o_lock_rsp(lock_rsp),
        .o_lock_rsp_VALID(lock_rsp_valid), .i_lock_rsp_READY(lock_rsp_ready),
        .i_cache_req(cache_req), .i_cache_req_VALID(cache_req_valid),
        .o_cache_req_READY(cache_req_ready), .o_cache_rsp(cache_rsp),
        .o_cache_rsp_VALID(cache_rsp_valid), .i_cache_rsp_READY(cache_rsp_ready),
        .o_cache_snp(cache_snp), .o_cache_snp_VALID(cache_snp_valid),
        .i_cache_snp_READY(cache_snp_ready), .i_cache_ack(cache_ack),
        .i_cache_ack_VALID(cache_ack_valid), .o_cache_ack_READY(cache_ack_ready),
        .o_lock_mem_req(lock_mem_req), .o_lock_mem_req_VALID(lock_mem_req_valid),
        .i_lock_mem_req_READY(lock_mem_req_ready), .i_lock_mem_rsp(lock_mem_rsp),
        .i_lock_mem_rsp_VALID(lock_mem_rsp_valid), .o_lock_mem_rsp_READY(lock_mem_rsp_ready),
        .i_formal_rsp_valid(f_rsp_valid), .i_formal_rsp(f_rsp),
        .i_formal_rsp_ready(f_rsp_ready), .o_formal_rsp_next_valid(f_rsp_next_valid),
        .o_formal_rsp_next(f_rsp_next), .i_formal_snp_valid(f_snp_valid),
        .i_formal_snp(f_snp), .i_formal_snp_ready(f_snp_ready),
        .o_formal_snp_next_valid(f_snp_next_valid), .o_formal_snp_next(f_snp_next)
    );

    `include "directory_env_stability.sv"
`ifndef VERILATOR
    `include "directory_stability_properties.sv"
`endif
endmodule

`default_nettype wire
