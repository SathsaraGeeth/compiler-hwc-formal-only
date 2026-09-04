`default_nettype none

module write_state_anvil (
    input  logic        i_clk,
    input  logic        i_rst_n,
    input  logic        f_attempt_valid,
    input  logic [1:0]  f_attempt_state,
    output logic        f_attempt_ready,
    output logic        f_write_commit,
    output logic [1:0]  f_commit_state
);
    logic [65:0] attempt;
    logic [65:0] commit;

    assign attempt       = {f_attempt_state, 64'b0};
    assign f_commit_state = commit[64 +: 2];

    write_state_coi dut (
        .clk_i               (i_clk),
        .rst_ni              (i_rst_n),
        ._port_attempt_ack   (f_attempt_ready),
        ._port_attempt_valid (f_attempt_valid),
        ._port_attempt_0     (attempt),
        ._port_commit_ack    (1'b1),
        ._port_commit_valid  (f_write_commit),
        ._port_commit_0      (commit)
    );

    `include "write_state_anvil_properties.sv"
endmodule

`default_nettype wire
