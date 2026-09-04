module formal_add(
    input  logic [7:0] a,
    input  logic [7:0] b,
    output logic [7:0] sum,
    output logic       carry
);
    assign {carry, sum} = a + b;
endmodule
