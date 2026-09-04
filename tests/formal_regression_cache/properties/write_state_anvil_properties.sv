property write_requires_writable_state_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!f_write_commit ||
            (f_commit_state == 2'd2) || (f_commit_state == 2'd3));
endproperty

write_requires_writable_state_anvil_assert:
    assert property (write_requires_writable_state_anvil);
