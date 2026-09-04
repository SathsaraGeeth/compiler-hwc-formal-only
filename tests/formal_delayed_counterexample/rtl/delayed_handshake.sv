module delayed_handshake (
    input  logic clk,
    input  logic rst_n,
    input  logic request,
    output logic grant
);

    logic [4:0] request_pipeline;

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            request_pipeline <= 5'b0;
            grant <= 1'b0;
        end else begin
            request_pipeline <= {request_pipeline[3:0], request};
            grant <= request_pipeline[4];
        end
    end

    // The design responds after six active edges, but the protocol requires
    // a response three edges after a request. A request therefore creates a
    // useful multi-cycle counterexample with clock, reset, request, pipeline,
    // and grant activity to inspect in the waveform.
    property request_gets_timely_grant;
        @(posedge clk) disable iff (!rst_n)
        request |-> ##3 grant;
    endproperty

    request_gets_timely_grant_assert:
        assert property (request_gets_timely_grant);

endmodule
