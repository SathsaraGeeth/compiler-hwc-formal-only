module multiclock_bridge (
    input  bit clk_source,
    input  bit clk_destination,
    input  bit rst_n,
    input  bit source_request,
    output bit synchronized_request,
    output bit destination_pulse
);

    bit synchronizer_stage;
    bit source_epoch;

    always_ff @(posedge clk_source) begin
        if (!rst_n)
            source_epoch <= 1'b0;
        else if (source_request)
            source_epoch <= !source_epoch;
    end

    always_ff @(posedge clk_destination) begin
        if (!rst_n) begin
            synchronizer_stage <= 1'b0;
            synchronized_request <= 1'b0;
            destination_pulse <= 1'b0;
        end else begin
            synchronizer_stage <= source_request;
            synchronized_request <= synchronizer_stage;
            destination_pulse <= synchronizer_stage && !synchronized_request;
        end
    end

    assume_source_request_stable:
        assume property (@(posedge clk_source) disable iff (!rst_n)
                         source_request |=> source_request);

    property destination_pulse_follows_synchronizer;
        @(posedge clk_destination) disable iff (!rst_n)
        always (destination_pulse implies synchronized_request);
    endproperty

    destination_pulse_follows_synchronizer_assert:
        assert property (destination_pulse_follows_synchronizer);

endmodule
