`default_nettype none

module fairness_anvil (
    input  logic i_clk,
    input  logic i_rst_n,
    output logic f_service0,
    output logic f_service1
);
    logic req0_ready, req1_ready;

    fairness_coi dut (
        .clk_i          (i_clk),
        .rst_ni         (i_rst_n),
        ._cpu0_req_ack  (req0_ready),
        ._cpu0_req_valid(1'b1),
        ._cpu0_req_0    (1'b0),
        ._cpu0_rsp_ack  (1'b1),
        ._cpu0_rsp_valid(f_service0),
        ._cpu0_rsp_0    (),
        ._cpu1_req_ack  (req1_ready),
        ._cpu1_req_valid(1'b1),
        ._cpu1_req_0    (1'b1),
        ._cpu1_rsp_ack  (1'b1),
        ._cpu1_rsp_valid(f_service1),
        ._cpu1_rsp_0    ()
    );

    `include "fairness_anvil_properties.sv"
endmodule

`default_nettype wire
