`timescale 1ns/1ps
`include "runtime.svh"

module circular_buffer_tb;
    localparam int DEPTH = 4;
    localparam int WIDTH = 8;

    logic                       clk;
    logic                       rst_n;
    logic [WIDTH-1:0]           head_data;
    logic                       deq_ready;
    logic                       deq_valid;
    logic [WIDTH-1:0]           enq_tail_data;
    logic                       enq_valid;
    logic                       enq_ready;
    logic                       full;
    logic                       empty;
    logic [$clog2(DEPTH+1)-1:0] level;
    logic                       clr_n;

    `runtime_stdin(clk)
    `runtime_stdin(rst_n)
    `runtime_stdin(deq_valid)
    `runtime_stdin(enq_tail_data)
    `runtime_stdin(enq_valid)
    `runtime_stdin(clr_n)

    circular_buffer #(
        .DEPTH (DEPTH),
        .WIDTH (WIDTH)
    ) dut (
        .i_clk           (clk),
        .i_rst_n         (rst_n),
        .o_head_data     (head_data),
        .o_deq_ready     (deq_ready),
        .i_deq_valid     (deq_valid),
        .i_enq_tail_data (enq_tail_data),
        .i_enq_valid     (enq_valid),
        .o_enq_ready     (enq_ready),
        .o_full          (full),
        .o_empty         (empty),
        .o_level         (level),
        .i_clr_n         (clr_n)
    );

    `runtime_stdout(head_data)
    `runtime_stdout(deq_ready)
    `runtime_stdout(enq_ready)
    `runtime_stdout(full)
    `runtime_stdout(empty)
    `runtime_stdout(level)
endmodule: circular_buffer_tb
