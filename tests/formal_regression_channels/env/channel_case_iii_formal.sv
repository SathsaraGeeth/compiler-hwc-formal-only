`default_nettype none
module channel_case_iii_formal (
    input logic i_clk, input logic i_rst_n,
    input logic f_req_valid, input logic f_rsp_ready,
    output logic f_accept, output logic [7:0] f_input_data,
    output logic f_out_valid, output logic [7:0] f_out_data
);
    logic req_ack, rsp_valid;
    logic [7:0] req_data, rsp_data;
    assign req_data = 8'h2a;
    imem dut (
        .clk_i(i_clk), .rst_ni(i_rst_n),
        ._ep_req_ack(req_ack), ._ep_req_valid(f_req_valid), ._ep_req_0(req_data),
        ._ep_rsp_ack(f_rsp_ready), ._ep_rsp_valid(rsp_valid), ._ep_rsp_0(rsp_data)
    );
    assign f_accept = f_req_valid && req_ack;
    assign f_input_data = req_data;
    assign f_out_valid = rsp_valid;
    assign f_out_data = rsp_data;
    `include "channel_transaction_checks.svh"
endmodule
`default_nettype wire
