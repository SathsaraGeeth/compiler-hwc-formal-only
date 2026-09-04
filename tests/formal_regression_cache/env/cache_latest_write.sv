`default_nettype none

import core_pkg::*;

module cache_latest_write (
    input  logic            i_clk,
    input  logic            i_rst_n,
    input  logic            f_write_commit,
    input  logic [XLEN-1:0] f_write_data,
    input  logic            f_read_complete,
    output logic [XLEN-1:0] f_read_data,
    output logic [XLEN-1:0] f_latest_write,
    output logic            f_latest_write_valid
);
    logic [XLEN-1:0] f_previous_write;
    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            f_latest_write       <= '0;
            f_latest_write_valid <= 1'b0;
            f_previous_write     <= '0;
        end else if (f_write_commit) begin
            f_previous_write     <= f_latest_write;
            f_latest_write       <= f_write_data;
            f_latest_write_valid <= 1'b1;
        end
    end

    always_comb begin
`ifndef KILL_LATEST_COHERENT_WRITE
`ifndef KILL_LATEST_COHERENT_WRITE_STALE
        f_read_data = f_latest_write;
`else
        f_read_data = f_previous_write;
`endif
`else
        f_read_data = f_latest_write ^ XLEN'(1);
`endif
    end

    `include "cache_env_latest_write.sv"
    `include "cache_latest_write_properties.sv"
endmodule

`default_nettype wire
