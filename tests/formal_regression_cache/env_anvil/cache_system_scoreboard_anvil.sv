`default_nettype none

module cache_system_scoreboard_anvil (
    input  logic i_clk,
    input  logic i_rst_n,
    output logic f_req_ready_0,
    output logic f_req_ready_1,
    output logic f_active_0, output logic f_active_1,
    output logic [57:0] f_line_0, output logic [57:0] f_line_1
);
    logic f_req_valid_0, f_req_valid_1;
    logic [2:0] f_cycle;
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) f_cycle <= '0;
        else if (f_cycle != 3'd7) f_cycle <= f_cycle + 1'b1;
    end
    assign f_req_valid_0 = (f_cycle >= 3'd1) && (f_cycle <= 3'd3);
    assign f_req_valid_1 = (f_cycle >= 3'd4);

    logic [57:0] same_line_req_0;
    logic [57:0] same_line_req_1;

    assign same_line_req_0 = '0;
    assign same_line_req_1 = '0;

    cache_system_scoreboard_coi dut (
        .clk_i          (i_clk),
        .rst_ni         (i_rst_n),
        ._cpu0_req_ack  (f_req_ready_0),
        ._cpu0_req_valid(f_req_valid_0),
        ._cpu0_req_0    (same_line_req_0),
        ._cpu1_req_ack  (f_req_ready_1),
        ._cpu1_req_valid(f_req_valid_1),
        ._cpu1_req_0    (same_line_req_1),
        .f_cpu0_active  (f_active_0), .f_cpu1_active(f_active_1),
        .f_cpu0_line    (f_line_0), .f_cpu1_line(f_line_1)
    );

    `include "cache_system_env_scoreboard_anvil.sv"
    `include "cache_system_scoreboard_anvil_properties.sv"
endmodule

`default_nettype wire
