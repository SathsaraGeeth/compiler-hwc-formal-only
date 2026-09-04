`default_nettype none

module latest_write_anvil (
    input logic i_clk,
    input logic i_rst_n,
    input logic f_req0_valid,
    input logic f_req0_store,
    input logic [63:0] f_req0_data,
    input logic f_req1_valid,
    input logic f_req1_store,
    input logic [63:0] f_req1_data,
    output logic f_rsp0_valid,
    output logic f_rsp1_valid,
    output logic f_rsp0_read,
    output logic f_rsp1_read,
    output logic [63:0] f_rsp0_data,
    output logic [63:0] f_rsp1_data,
    output logic [63:0] f_rsp0_expected,
    output logic [63:0] f_rsp1_expected,
    output logic f_bad_read_seen
);
    logic req0_ready, req1_ready;
    logic req0_valid, req1_valid;
    logic [128:0] rsp0, rsp1;
    logic r_store_accepted;
    logic f_reset_n;

`ifdef SYMBIYOSYS_COMPATIBLE
    logic [1:0] r_formal_reset = 2'b00;
    always_ff @(posedge i_clk) begin
        if (!r_formal_reset[1])
            r_formal_reset <= r_formal_reset + 1'b1;
    end
    assign f_reset_n = r_formal_reset[1];
`else
    assign f_reset_n = i_rst_n;
`endif

    // Force a non-vacuous write-then-read execution. Client 0 first stores an
    // arbitrary formal value. Client 1 becomes valid only after that store is
    // accepted and subsequently reads the same abstract line.
    assign req0_valid = !r_store_accepted;
    assign req1_valid = r_store_accepted;

    always_ff @(posedge i_clk) begin
        if (!f_reset_n)
            r_store_accepted <= 1'b0;
        else if (req0_valid && req0_ready)
            r_store_accepted <= 1'b1;
    end

    assign f_rsp0_read = rsp0[128];
    assign f_rsp1_read = rsp1[128];
    assign f_rsp0_data = rsp0[64 +: 64];
    assign f_rsp1_data = rsp1[64 +: 64];
    assign f_rsp0_expected = rsp0[0 +: 64];
    assign f_rsp1_expected = rsp1[0 +: 64];

    always_ff @(posedge i_clk) begin
        if (!f_reset_n)
            f_bad_read_seen <= 1'b0;
        else if ((f_rsp0_valid && f_rsp0_read && (f_rsp0_data != f_rsp0_expected)) ||
                 (f_rsp1_valid && f_rsp1_read && (f_rsp1_data != f_rsp1_expected)))
            f_bad_read_seen <= 1'b1;
    end

    latest_write_coi dut (
        .clk_i          (i_clk),
        .rst_ni         (f_reset_n),
        ._cpu0_req_ack  (req0_ready),
        ._cpu0_req_valid(req0_valid),
        ._cpu0_req_0    ({1'b1, f_req0_data}),
        ._cpu0_rsp_ack  (1'b1),
        ._cpu0_rsp_valid(f_rsp0_valid),
        ._cpu0_rsp_0    (rsp0),
        ._cpu1_req_ack  (req1_ready),
        ._cpu1_req_valid(req1_valid),
        ._cpu1_req_0    ({1'b0, 64'b0}),
        // Hold the read response under backpressure. This keeps the completed
        // read observation visible at the selected BMC bound and also checks
        // that the generated dynamic channel retains its payload.
        ._cpu1_rsp_ack  (1'b0),
        ._cpu1_rsp_valid(f_rsp1_valid),
        ._cpu1_rsp_0    (rsp1)
    );

`ifdef FORMAL
    `include "latest_write_anvil_properties.sv"
`endif
endmodule

`default_nettype wire
