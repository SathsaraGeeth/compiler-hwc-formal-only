property matching_ack_precedes_grant_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !(f_rsp_valid && f_snoop_required &&
             !(f_matching_ack_seen || (f_ack_valid && f_ack_ready)));
endproperty

matching_ack_precedes_grant_anvil_assert:
    assert property (matching_ack_precedes_grant_anvil);
