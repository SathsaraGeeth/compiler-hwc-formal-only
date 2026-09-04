`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        write_requires_writable_state_assert:
            assert (!f_write_commit || f_writable_state);
`else
property write_requires_writable_state;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!f_write_commit || f_writable_state);
endproperty

write_requires_writable_state_assert:
    assert property (write_requires_writable_state);
`endif
