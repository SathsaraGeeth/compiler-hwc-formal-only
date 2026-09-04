`default_nettype none

module directory_ack_order_anvil (
    input logic i_clk,
    input logic i_rst_n,
    input logic f_req_valid,
    input logic f_ack_valid,
    output logic f_rsp_valid,
    output logic f_ack_ready,
    output logic f_snoop_required,
    output logic f_matching_ack_seen
);

    logic req_ready;
    logic snoop_valid;

    directory_ack_order_coi dut (
        .clk_i                    (i_clk),
        .rst_ni                   (i_rst_n),
        ._requester_req_ack       (req_ready),
        ._requester_req_valid     (f_req_valid),
        ._requester_req_0         ('0),
        ._requester_rsp_ack       (1'b1),
        ._requester_rsp_valid     (f_rsp_valid),
        ._requester_rsp_0         (),
        ._owner_snoop_ack         (1'b1),
        ._owner_snoop_valid       (snoop_valid),
        ._owner_snoop_0           (),
        ._owner_snoop_rsp_ack     (f_ack_ready),
        ._owner_snoop_rsp_valid   (f_ack_valid),
        ._owner_snoop_rsp_0       ('0)
    );

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            f_snoop_required    <= 1'b0;
            f_matching_ack_seen <= 1'b0;
        end else begin
            if (snoop_valid) begin
                f_snoop_required    <= 1'b1;
                f_matching_ack_seen <= 1'b0;
            end
            if (f_ack_valid && f_ack_ready)
                f_matching_ack_seen <= 1'b1;
            if (f_rsp_valid)
                f_snoop_required <= 1'b0;
        end
    end

    `include "directory_env_ack_order_anvil.sv"
    `include "directory_ack_order_anvil_properties.sv"
endmodule

`default_nettype wire
