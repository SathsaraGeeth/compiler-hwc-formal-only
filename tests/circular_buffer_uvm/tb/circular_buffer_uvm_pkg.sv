`timescale 1ns/1ps

package circular_buffer_uvm_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    localparam int DEPTH = 4;
    localparam int WIDTH = 8;

    logic clk;
    logic rst_n;
    logic [WIDTH-1:0] head_data;
    logic deq_ready;
    logic deq_valid;
    logic [WIDTH-1:0] enq_tail_data;
    logic enq_valid;
    logic enq_ready;
    logic full;
    logic empty;
    logic [$clog2(DEPTH+1)-1:0] level;
    logic clr_n;

    class circular_buffer_test extends uvm_test;
        `uvm_component_utils(circular_buffer_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            clk = 1'b0;
            rst_n = 1'b0;
            deq_valid = 1'b0;
            enq_tail_data = '0;
            enq_valid = 1'b0;
            clr_n = 1'b1;

            #1 clk = 1'b1;
            #1 clk = 1'b0;
            rst_n = 1'b1;

            enq_tail_data = 8'h2a;
            enq_valid = 1'b1;
            #1 clk = 1'b1;
            #1 clk = 1'b0;
            enq_valid = 1'b0;
            #1;
            assert (level == 1);
            assert (!empty);
            assert (head_data == 8'h2a);

            deq_valid = 1'b1;
            #1 clk = 1'b1;
            #1 clk = 1'b0;
            deq_valid = 1'b0;
            #1;
            assert (level == 0);
            assert (empty);
            phase.drop_objection(this);
        endtask
    endclass
endpackage
