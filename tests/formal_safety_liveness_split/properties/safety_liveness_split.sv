module safety_liveness_split;
  logic clk;
  logic req;
  logic ack;
  logic done;
  logic [127:0] state;
  logic [127:0] signature;
  logic [5:0] phase;

  split_dut dut(
    .clk(clk), .req(req), .ack(ack), .done(done),
    .state(state), .signature(signature), .phase(phase)
  );

  property request_and_progress;
    @(posedge clk)
      (always (req |-> (ack && signature == state))) and
      (s_eventually done);
  endproperty

  request_and_progress_assert: assert property (request_and_progress);

  property trivial_safety_and_liveness;
    @(posedge clk)
      (always (signature == state)) and
      (s_eventually (signature == state));
  endproperty

  trivial_safety_and_liveness_assert:
    assert property (trivial_safety_and_liveness);
endmodule
