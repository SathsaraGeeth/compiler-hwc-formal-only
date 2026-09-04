`default_nettype none
module directory_owner_anvil (
    input logic i_clk,
    input logic i_rst_n,
    input logic f_req_valid,
    input logic [1:0] f_req,
    output logic f_req_ready,
    output logic f_owner_valid,
    output logic [1:0] f_sharers
);
    directory_owner_coi dut (
        .clk_i(i_clk), .rst_ni(i_rst_n),
        ._cache_req_ack(f_req_ready),
        ._cache_req_valid(f_req_valid),
        ._cache_req_0(f_req),
        .f_owner_valid(f_owner_valid), .f_sharers(f_sharers)
    );
    `include "directory_owner_anvil_properties.sv"
endmodule
`default_nettype wire
