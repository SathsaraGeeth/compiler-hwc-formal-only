`default_nettype none

module channel_stability_anvil (
    input logic i_clk,
    input logic i_rst_n,
    input logic f_req_valid,
    input logic f_rsp_ready,
    output logic f_rsp_valid,
    output logic [71:0] f_rsp_data,
    output logic f_was_stalled,
    output logic [71:0] f_stalled_data
);
    channel_stability_coi dut (
        .clk_i         (i_clk),
        .rst_ni        (i_rst_n),
        ._port_req_ack (),
        ._port_req_valid(f_req_valid),
        ._port_req_0   ('0),
        ._port_rsp_ack (f_rsp_ready),
        ._port_rsp_valid(f_rsp_valid),
        ._port_rsp_0   (f_rsp_data)
    );

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            f_was_stalled <= 1'b0;
            f_stalled_data <= '0;
        end else begin
            f_was_stalled <= f_rsp_valid && !f_rsp_ready;
            if (f_rsp_valid && !f_rsp_ready)
                f_stalled_data <= f_rsp_data;
        end
    end

    `include "channel_env_stability_anvil.sv"
    `include "channel_stability_anvil_properties.sv"
endmodule

`default_nettype wire
