property exclusive_owner_has_no_sharers_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !(f_owner_valid && (f_sharers != 2'b00));
endproperty
exclusive_owner_has_no_sharers_anvil_assert:
    assert property (exclusive_owner_has_no_sharers_anvil);
