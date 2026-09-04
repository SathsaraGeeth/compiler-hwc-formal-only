`default_nettype none

import core_pkg::*;

module cache_transient_state (
    input  logic                  i_clk,
    input  logic                  i_rst_n,
    input  logic                  f_upstream_valid,
    input  logic                  f_stage_ready,
    input  logic                  f_line_valid,
    input  logic [PHY_ADDR_W-1:0] f_upstream_addr,
    input  logic                  f_upstream_requester,
    input  logic [3:0]            f_upstream_trans_id,
    output logic                  f_stage_valid,
    output logic                  f_stage_fire,
    output logic [PHY_ADDR_W-1:0] f_stage_addr,
    output logic                  f_stage_requester,
    output logic [3:0]            f_stage_trans_id
);
    logic                  stage_valid_next;
    logic [PHY_ADDR_W-1:0] stage_addr_next;
    logic                  stage_requester_next;
    logic [3:0]            stage_trans_id_next;

    assign f_stage_fire = f_stage_valid && f_line_valid && f_stage_ready;

    always_comb begin
        stage_valid_next     = f_stage_valid;
        stage_addr_next      = f_stage_addr;
        stage_requester_next = f_stage_requester;
        stage_trans_id_next  = f_stage_trans_id;

        if (!f_stage_valid || f_stage_fire) begin
            stage_valid_next = f_upstream_valid;
            if (f_upstream_valid) begin
                stage_addr_next      = f_upstream_addr;
                stage_requester_next = f_upstream_requester;
                stage_trans_id_next  = f_upstream_trans_id;
            end
`ifdef KILL_TRANSIENT_STATE_STABILITY
        end else if (!f_line_valid) begin
            stage_trans_id_next = f_stage_trans_id + 1'b1;
`endif
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            f_stage_valid     <= 1'b0;
            f_stage_addr      <= '0;
            f_stage_requester <= '0;
            f_stage_trans_id  <= '0;
        end else begin
            f_stage_valid     <= stage_valid_next;
            f_stage_addr      <= stage_addr_next;
            f_stage_requester <= stage_requester_next;
            f_stage_trans_id  <= stage_trans_id_next;
        end
    end

    `include "cache_env_transient_state.sv"
    `include "cache_transient_state_properties.sv"
endmodule

`default_nettype wire
