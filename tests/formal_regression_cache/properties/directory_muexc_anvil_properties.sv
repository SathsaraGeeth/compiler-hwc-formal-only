property muexc_cache_req_accepted_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !((f_req_valid_0 && f_req_ready_0) &&
             (f_req_valid_1 && f_req_ready_1));
endproperty

muexc_cache_req_accepted_anvil_assert:
    assert property (muexc_cache_req_accepted_anvil);
