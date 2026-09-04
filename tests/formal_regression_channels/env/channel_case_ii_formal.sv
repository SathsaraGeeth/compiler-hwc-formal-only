`default_nettype none
module channel_case_ii_formal (
    input logic i_clk, input logic i_rst_n,
    input logic f_req_valid, input logic [7:0] f_req_data, input logic f_rsp_ready,
    output logic f_accept, output logic [7:0] f_input_data,
    output logic f_out_valid, output logic [7:0] f_out_data
);
    logic req_ack, rsp_valid;
    logic [7:0] rsp_data;
    imem dut (
        .clk_i(i_clk), .rst_ni(i_rst_n),
        ._memory_req_ack(req_ack), ._memory_req_valid(f_req_valid), ._memory_req_0(f_req_data),
        ._memory_rsp_ack(f_rsp_ready), ._memory_rsp_valid(rsp_valid), ._memory_rsp_0(rsp_data)
    );
    assign f_accept = f_req_valid && req_ack;
    assign f_input_data = f_req_data;
    assign f_out_valid = rsp_valid;
    assign f_out_data = rsp_data;
    `include "channel_transaction_checks.svh"
endmodule
`default_nettype wire
