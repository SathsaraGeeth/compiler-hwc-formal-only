`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        assume (!(f_owner_valid && (f_sharers != '0)));
`else
assume property (@(posedge i_clk) disable iff (!i_rst_n)
    always !(f_owner_valid && (f_sharers != '0)));
`endif
