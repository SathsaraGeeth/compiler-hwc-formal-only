`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        assume (!(f_write_commit && f_read_complete));
`else
assume property (@(posedge i_clk) disable iff (!i_rst_n)
    always !(f_write_commit && f_read_complete));
`endif
