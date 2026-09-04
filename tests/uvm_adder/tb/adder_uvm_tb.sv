module adder_uvm_tb;
    import uvm_pkg::*;
    import adder_uvm_pkg::*;

    add dut(.clk(clk), .a(a), .b(b), .sum(sum), .carry(carry));

    initial begin
        clk = 1'b0;
        run_test("adder_test");
    end
endmodule
