`default_nettype none

module directory_muexc_anvil (
    input  logic i_clk,
    input  logic i_rst_n,
    input  logic f_req_valid_0,
    input  logic f_req_valid_1,
    output logic f_req_ready_0,
    output logic f_req_ready_1
);

    logic [578:0] req_0;
    logic [578:0] req_1;

    assign req_0 = '0;
    assign req_1 = '0;

    directory dut (
        .clk_i                    (i_clk),
        .rst_ni                   (i_rst_n),
        ._cache0_req_ack          (f_req_ready_0),
        ._cache0_req_valid        (f_req_valid_0),
        ._cache0_req_0            (req_0),
        ._cache0_rsp_ack          (1'b1),
        ._cache0_rsp_valid        (),
        ._cache0_rsp_0            (),
        ._cache0_snoop_ack        (1'b1),
        ._cache0_snoop_valid      (),
        ._cache0_snoop_0          (),
        ._cache0_snoop_rsp_ack    (),
        ._cache0_snoop_rsp_valid  (1'b0),
        ._cache0_snoop_rsp_0      ('0),
        ._cache1_req_ack          (f_req_ready_1),
        ._cache1_req_valid        (f_req_valid_1),
        ._cache1_req_0            (req_1),
        ._cache1_rsp_ack          (1'b1),
        ._cache1_rsp_valid        (),
        ._cache1_rsp_0            (),
        ._cache1_snoop_ack        (1'b1),
        ._cache1_snoop_valid      (),
        ._cache1_snoop_0          (),
        ._cache1_snoop_rsp_ack    (),
        ._cache1_snoop_rsp_valid  (1'b0),
        ._cache1_snoop_rsp_0      ('0),
        ._memory_req_ack          (1'b1),
        ._memory_req_valid        (),
        ._memory_req_0            (),
        ._memory_rsp_ack          (),
        ._memory_rsp_valid        (1'b1),
        ._memory_rsp_0            ('0)
    );

    `include "directory_env_muexc_anvil.sv"
    `include "directory_muexc_anvil_properties.sv"
endmodule

`default_nettype wire
