`include "runtime.svh"

module adder_tb;
    logic       clk;
    logic [7:0] a;
    logic [7:0] b;
    logic [7:0] sum;
    logic       carry;

    `runtime_stdin(clk)
    `runtime_stdin(a)
    `runtime_stdin(b)

    assign b = 8'd3;

    add dut (
        .clk   (clk),
        .a     (a),
        .b     (b),
        .sum   (sum),
        .carry (carry)
    );

    `runtime_stdout(sum)
    `runtime_stdout(carry)

    initial begin
        clk = 0;
        a = 5;
        #1 clk = 1;
        #1;
        assert ({carry, sum} == 9'd8);
        $finish;
    end

endmodule: adder_tb
