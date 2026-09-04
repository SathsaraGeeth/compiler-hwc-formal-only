package adder_uvm_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    logic clk;
    logic [7:0] a;
    logic [7:0] b;
    logic [7:0] sum;
    logic carry;

    class adder_test extends uvm_test;
        `uvm_component_utils(adder_test)
        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            a = 8'd5;
            b = 8'd3;
            #1 clk = 1'b1;
            #1;
            assert ({carry, sum} == 9'd8);
            phase.drop_objection(this);
        endtask
    endclass
endpackage
