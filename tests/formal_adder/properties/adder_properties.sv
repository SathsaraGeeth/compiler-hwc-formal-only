module formal_adder_properties;
    logic [7:0] a;
    logic [7:0] b;
    logic [7:0] sum;
    logic carry;

    formal_add dut(.a(a), .b(b), .sum(sum), .carry(carry));

    property adder_correct;
        @(a or b or sum or carry)
        {carry, sum} == ({1'b0, a} + {1'b0, b});
    endproperty

    adder_correct_assert: assert property (adder_correct);
endmodule
