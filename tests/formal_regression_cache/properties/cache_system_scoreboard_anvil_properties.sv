property unique_outstanding_line_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !(f_active_0 && f_active_1 && (f_line_0 == f_line_1));
endproperty
unique_outstanding_line_anvil_assert:
    assert property (unique_outstanding_line_anvil);
