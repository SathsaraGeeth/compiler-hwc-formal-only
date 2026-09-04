`default_nettype none
module transient_state_anvil (
    input logic i_clk,
    input logic i_rst_n,
    input logic [72:0] f_req,
    output logic f_req_ready,
    output logic f_done_ready,
    output logic f_waiting,
    output logic [72:0] f_current_request
);
    logic f_req_valid;
    logic f_done_valid;
    logic f_issued;
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) f_issued <= 1'b0;
        else if (f_req_valid && f_req_ready) f_issued <= 1'b1;
    end
    assign f_req_valid = !f_issued;
    assign f_done_valid = 1'b0;
    logic f_past_valid;
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) f_past_valid <= 1'b0;
        else f_past_valid <= 1'b1;
    end
    transient_state_coi dut (
        .clk_i(i_clk), .rst_ni(i_rst_n),
        ._port_req_ack(f_req_ready),
        ._port_req_valid(f_req_valid),
        ._port_req_0(f_req),
        ._port_done_ack(f_done_ready),
        ._port_done_valid(f_done_valid),
        ._port_done_0('0),
        .f_waiting(f_waiting), .f_current_request(f_current_request)
    );
    `include "transient_state_anvil_properties.sv"
endmodule
`default_nettype wire
