module btor2_pipeline (
    input logic clk,
    input logic rst_n,
    input logic request,
    output logic grant
);

    always_ff @(posedge clk) begin
        if (!rst_n)
            grant <= 1'b0;
        else
            grant <= request;
    end

    property request_protocol;
        @(posedge clk) disable iff (!rst_n)
        (always (grant implies $past(request))) and
        (always (request implies s_eventually grant));
    endproperty

    request_protocol_assert:
        assert property (request_protocol);

    // this should fail
    property failing_grant_never;
        @(posedge clk) disable iff (!rst_n) always (!grant);
    endproperty

    failing_grant_never_assert:
        assert property (failing_grant_never);

endmodule
