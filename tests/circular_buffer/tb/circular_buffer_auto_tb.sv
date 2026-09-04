`timescale 1ns/1ps

module circular_buffer_auto_tb;
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

    initial begin
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
        assert (deq_ready);
        assert (head_data == 8'h2a);

        deq_valid = 1'b1;
        #1 clk = 1'b1;
        #1 clk = 1'b0;
        deq_valid = 1'b0;
        #1;
        assert (level == 0);
        assert (empty);
        assert (!deq_ready);
        $finish;
    end
endmodule: circular_buffer_auto_tb
