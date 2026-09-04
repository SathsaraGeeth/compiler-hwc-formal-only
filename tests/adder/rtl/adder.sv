module add(
    input  logic       clk,
    input  logic [7:0] a,
    input  logic [7:0] b,
    output logic [7:0] sum,
    output logic       carry
);

always_ff @(posedge clk) begin
     {carry,sum} <= a+b;
end

endmodule: add
