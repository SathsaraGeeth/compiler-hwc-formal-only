`default_nettype none

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;
import directory_pkg::*;

module directory_type_fair (
    input  logic i_clk,
    input  logic i_rst_n,
    input  logic f_cacheable_valid,
    input  logic f_uncacheable_valid,
    input  logic f_cache_rsp_ready,
    input  logic f_cache_snp_ready,
    input  logic f_lock_rsp_ready,
    input  logic f_mem_req_ready,
    input  logic f_mem_rsp_valid,
    output logic f_uncacheable_ready
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
    logic [1:0] r_cacheable_line;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            r_cacheable_line <= '0;
        end else if (f_cacheable_valid) begin
            r_cacheable_line <= r_cacheable_line + 1'b1;
        end
    end

    always_comb begin
        for (int i = 0; i < NUM_CACHES; i++) begin
            i_cpu_req[i]         = '0;
            i_cpu_req_VALID[i]   = 1'b0;
            i_cpu_req_READY[i]   = 1'b0;
            i_cpu_rsp[i]         = '0;
            i_cpu_rsp_VALID[i]   = 1'b0;
            i_cpu_rsp_READY[i]   = 1'b1;
            i_lock_req[i]        = '0;
            i_lock_req_VALID[i]  = 1'b0;
            i_lock_rsp_READY[i]  = f_lock_rsp_ready;
            i_cache_req[i]       = '0;
            i_cache_req_VALID[i] = 1'b0;
            i_cache_rsp_READY[i] = f_cache_rsp_ready;
            i_cache_snp_READY[i] = f_cache_snp_ready;
            i_cache_ack[i]       = '0;
            i_cache_ack_VALID[i] = 1'b0;
        end

        i_lock_req[1].op      = CPU_AMO_ADD;
        i_lock_req[1].mem_type = MEM_UC;
        i_lock_req[1].addr    = 64'h0080;
        i_lock_req[1].size    = 3;
        i_lock_req_VALID[1]   = f_uncacheable_valid;
        i_cache_req[0].addr   = {
            {(PHY_ADDR_W-2-BYTE_OFFSET_W){1'b0}},
            r_cacheable_line,
            {BYTE_OFFSET_W{1'b0}}
        };
        i_cache_req[0].src_id = 0;
        i_cache_req[0].msg    = GETS;
        i_cache_req_VALID[0]  = f_cacheable_valid;
        i_lock_mem_req_READY  = f_mem_req_ready;
        i_lock_mem_rsp        = '0;
        i_lock_mem_rsp_VALID  = f_mem_rsp_valid;
    end

    directory #(
        .NUM_DIR_ENTRIES(4),
        .NUM_SCOREBOARD_ENTRIES(2)
    ) dut (.*);

    assign f_uncacheable_ready = o_lock_req_READY[1];

    `include "directory_env_type_fair.sv"
    `include "directory_type_fair_properties.sv"
endmodule

`default_nettype wire
