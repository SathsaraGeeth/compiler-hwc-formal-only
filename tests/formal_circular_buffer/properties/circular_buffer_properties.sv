`timescale 1ns/1ps

module circular_buffer_properties;
    localparam int DEPTH = 4;
    localparam int WIDTH = 8;

    logic                         i_clk;
    logic                         i_rst_n;
    logic [WIDTH-1:0]             o_head_data;
    logic                         o_deq_ready;
    logic                         i_deq_valid;
    logic [WIDTH-1:0]             i_enq_tail_data;
    logic                         i_enq_valid;
    logic                         o_enq_ready;
    logic                         o_full;
    logic                         o_empty;
    logic [$clog2(DEPTH+1)-1:0]  o_level;
    logic                         i_clr_n;

    circular_buffer #(
        .DEPTH (DEPTH),
        .WIDTH (WIDTH)
    ) dut (
        .i_clk           (i_clk),
        .i_rst_n         (i_rst_n),
        .o_head_data     (o_head_data),
        .o_deq_ready     (o_deq_ready),
        .i_deq_valid     (i_deq_valid),
        .i_enq_tail_data (i_enq_tail_data),
        .i_enq_valid     (i_enq_valid),
        .o_enq_ready     (o_enq_ready),
        .o_full          (o_full),
        .o_empty         (o_empty),
        .o_level         (o_level),
        .i_clr_n         (i_clr_n)
    );

    property reset_clears;
        @(posedge i_clk)
        (!i_rst_n || !i_clr_n) |=>
            (o_level == 3'd0 && o_empty && !o_full && !o_deq_ready);
    endproperty

    property flags_match_level;
        @(posedge i_clk)
        (o_level <= 3'd4) |->
            (o_empty == (o_level == 3'd0) &&
             o_full == (o_level == 3'd4) &&
             o_enq_ready == !o_full);
    endproperty

    property level_remains_bounded;
        @(posedge i_clk)
        disable iff (!i_rst_n || !i_clr_n)
        (o_level <= 3'd4 && o_deq_ready == (o_level != 3'd0)) |=>
            (o_level <= 3'd4);
    endproperty

    property enqueue_increments_level;
        @(posedge i_clk)
        disable iff (!i_rst_n || !i_clr_n)
        (o_level < 3'd4 &&
         i_enq_valid && !i_deq_valid) |=>
            (o_level == ($past(o_level) + 3'd1));
    endproperty

    property dequeue_decrements_level;
        @(posedge i_clk)
        disable iff (!i_rst_n || !i_clr_n)
        (o_level > 3'd0 && o_deq_ready &&
         i_deq_valid && !i_enq_valid) |=>
            (o_level == ($past(o_level) - 3'd1));
    endproperty

    property reach_full;
        @(posedge i_clk)
        (!i_rst_n || !i_clr_n) |-> ##[1:12]
            (i_rst_n && i_clr_n &&
             $past(o_level) == 3'd3 && o_full);
    endproperty

    property reach_simultaneous_transfer;
        @(posedge i_clk)
        (!i_rst_n || !i_clr_n) |-> ##[1:12]
            (i_rst_n && i_clr_n &&
             $past(o_level) > 3'd0 &&
             i_enq_valid && o_enq_ready &&
             i_deq_valid && o_deq_ready);
    endproperty

    assert property (reset_clears);
    assert property (flags_match_level);
    assert property (level_remains_bounded);
    assert property (enqueue_increments_level);
    assert property (dequeue_decrements_level);

    cover property (reach_full);
    cover property (reach_simultaneous_transfer);
endmodule
