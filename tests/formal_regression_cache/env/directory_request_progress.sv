`default_nettype none

import core_pkg::*;
import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;

module directory_request_progress (
    input  logic                  i_clk,
    input  logic                  i_rst_n,
    input  logic                  f_req_valid,
    input  coh_req_e              f_req_msg,
    input  logic [CACHE_ID_W-1:0] f_req_src_id,
    input  logic [TRANS_ID_W-1:0] f_req_trans_id,
    input  logic                  f_rsp_ready,
    output logic                  f_req_ready,
    output logic                  f_accepted_get,
    output logic                  f_rsp_valid,
    output logic [CACHE_ID_W-1:0] f_rsp_dst_id,
    output logic [TRANS_ID_W-1:0] f_rsp_trans_id
);
    logic pending;
    logic [CACHE_ID_W-1:0] pending_dst_id;
    logic [TRANS_ID_W-1:0] pending_trans_id;

    assign f_req_ready   = !pending;
    assign f_accepted_get = f_req_valid && f_req_ready &&
                           ((f_req_msg == GETS) || (f_req_msg == GETM));
`ifndef KILL_REQUEST_RESPONSE_PROGRESS
    assign f_rsp_valid   = pending;
`else
    assign f_rsp_valid   = 1'b0;
`endif
    assign f_rsp_dst_id  = pending_dst_id;
    assign f_rsp_trans_id = pending_trans_id;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            pending          <= 1'b0;
            pending_dst_id   <= '0;
            pending_trans_id <= '0;
        end else begin
            if (f_rsp_valid && f_rsp_ready) begin
                pending <= 1'b0;
            end
            if (f_accepted_get) begin
                pending          <= 1'b1;
                pending_dst_id   <= f_req_src_id;
                pending_trans_id <= f_req_trans_id;
            end
        end
    end

    `include "directory_env_request_progress.sv"
    `include "directory_request_progress_properties.sv"
endmodule

`default_nettype wire
