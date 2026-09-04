module split_dut(
  input logic clk,
  input logic req,
  input logic ack,
  input logic done,
  output logic [127:0] state,
  output logic [127:0] signature,
  output logic [5:0] phase
);

  always_ff @(posedge clk) begin
    state <= {state[126:0], state[127] ^ state[95] ^ state[62] ^ state[0]};
    phase <= phase + 6'd1;
  end

  assign signature = state;
endmodule
