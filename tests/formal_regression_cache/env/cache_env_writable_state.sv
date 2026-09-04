`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        assume ((f_line_state == LINE_I) || (f_line_state == LINE_E) ||
                (f_line_state == LINE_S) || (f_line_state == LINE_M));
`else
assume property (@(posedge i_clk) disable iff (!i_rst_n)
    always ((f_line_state == LINE_I) || (f_line_state == LINE_E) ||
            (f_line_state == LINE_S) || (f_line_state == LINE_M)));
`endif
