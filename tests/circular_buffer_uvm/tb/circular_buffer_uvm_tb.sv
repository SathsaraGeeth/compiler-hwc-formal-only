module circular_buffer_uvm_tb;
    import uvm_pkg::*;
    import circular_buffer_uvm_pkg::*;

    circular_buffer #(
        .DEPTH(DEPTH),
        .WIDTH(WIDTH)
    ) dut (
        .i_clk(clk),
        .i_rst_n(rst_n),
        .o_head_data(head_data),
        .o_deq_ready(deq_ready),
        .i_deq_valid(deq_valid),
        .i_enq_tail_data(enq_tail_data),
        .i_enq_valid(enq_valid),
        .o_enq_ready(enq_ready),
        .o_full(full),
        .o_empty(empty),
        .o_level(level),
        .i_clr_n(clr_n)
    );

    initial run_test("circular_buffer_test");
endmodule
