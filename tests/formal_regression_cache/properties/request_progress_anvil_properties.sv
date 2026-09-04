property accepted_request_eventually_responds_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (f_request_accepted implies s_eventually
        (f_response && (f_response_id == 8'ha5)));
endproperty

accepted_request_eventually_responds_anvil_assert:
    assert property (accepted_request_eventually_responds_anvil);
