`default_nettype none

module request_progress_anvil (
    input  logic i_clk,
    input  logic i_rst_n,
    output logic f_request_accepted,
    output logic f_response,
    output logic [7:0] f_response_id
);
    logic request_ready;
    logic [9:0] request;
    logic [7:0] response;

    assign f_request_accepted = request_ready;
    assign request = {8'ha5, 2'd0};
    assign f_response_id = response;

    request_progress_coi dut (
        .clk_i                  (i_clk),
        .rst_ni                 (i_rst_n),
        ._cache_req_ack         (request_ready),
        ._cache_req_valid       (1'b1),
        ._cache_req_0           (request),
        ._cache_rsp_ack         (1'b1),
        ._cache_rsp_valid       (f_response),
        ._cache_rsp_0           (response)
    );

    `include "request_progress_anvil_properties.sv"
endmodule

`default_nettype wire
