property retained_transaction_stable_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!f_past_valid ||
            !$past(f_waiting) ||
            (f_waiting && (f_current_request == $past(f_current_request))));
endproperty
retained_transaction_stable_anvil_assert:
    assert property (retained_transaction_stable_anvil);
