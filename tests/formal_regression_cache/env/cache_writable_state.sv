`default_nettype none

import core_pkg::*;
import coherency_pkg::*;

module cache_writable_state (
    input  logic        i_clk,
    input  logic        i_rst_n,
    input  logic        f_req_valid,
    input  logic        f_req_write,
    input  line_state_e f_line_state,
    output logic        f_write_commit,
    output logic        f_writable_state
);
    always_comb begin
        f_writable_state = f_line_state inside {LINE_M, LINE_E};
`ifndef KILL_WRITABLE_STATE_CHECK
`ifndef KILL_WRITABLE_STATE_ALLOW_SHARED
        f_write_commit = f_req_valid && f_req_write && f_writable_state;
`else
        f_write_commit = f_req_valid && f_req_write &&
                         (f_writable_state || (f_line_state == LINE_S));
`endif
`else
        f_write_commit = f_req_valid && f_req_write;
`endif
    end

    `include "cache_env_writable_state.sv"
    `include "cache_writable_state_properties.sv"
endmodule

`default_nettype wire
