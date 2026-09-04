`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        directory_exclusive_owner_invariant_assert:
            assert (!f_exclusive_violation);
`else
property directory_exclusive_owner_invariant;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !f_exclusive_violation;
endproperty

assert property (directory_exclusive_owner_invariant);
`endif
