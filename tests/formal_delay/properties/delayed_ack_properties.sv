module delayed_ack_properties;
    logic clk;
    logic req;
    logic ack;

    delayed_ack dut(.clk(clk), .req(req), .ack(ack));

    property delayed_ack_correct;
        @(posedge clk) req |-> ##1 ack;
    endproperty

    property delayed_ack_wrong;
        @(posedge clk) req |-> ##1 !ack;
    endproperty

    property delayed_ack_window;
        @(posedge clk) req |-> ##[1:3] ack;
    endproperty

    property sampled_history;
        @(posedge clk) $rose(req) |-> ##1 $past(req);
    endproperty

    assert property (delayed_ack_correct);
    assert property (delayed_ack_wrong);
    assert property (delayed_ack_window);
    assert property (sampled_history);
endmodule
