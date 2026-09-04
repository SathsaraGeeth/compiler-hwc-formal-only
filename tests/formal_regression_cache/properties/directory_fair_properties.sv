/* FG(req) -> GF(accept) */
property cache_0_justice;
    @(posedge i_clk) disable iff (!i_rst_n)
    (s_eventually (always f_req_valid_0)) implies
        (always (s_eventually
            (f_req_valid_0 && f_req_ready_0)));
endproperty

property cache_1_justice;
    @(posedge i_clk) disable iff (!i_rst_n)
    (s_eventually (always f_req_valid_1)) implies
        (always (s_eventually
            (f_req_valid_1 && f_req_ready_1)));
endproperty

/* GF(req) -> GF(accept) */
property cache_0_strong_fairness;
    @(posedge i_clk) disable iff (!i_rst_n)
    (always (s_eventually f_req_valid_0)) implies
        (always (s_eventually
            (f_req_valid_0 && f_req_ready_0)));
endproperty

property cache_1_strong_fairness;
    @(posedge i_clk) disable iff (!i_rst_n)
    (always (s_eventually f_req_valid_1)) implies
        (always (s_eventually
            (f_req_valid_1 && f_req_ready_1)));
endproperty

property directory_both_clients_accepted;
    @(posedge i_clk) disable iff (!i_rst_n)
    (f_req_valid_0 && f_req_ready_0) ##[1:8]
        (f_req_valid_1 && f_req_ready_1);
endproperty

cache_0_justice_assert:
    assert property (cache_0_justice);
cache_1_justice_assert:
    assert property (cache_1_justice);
cache_0_strong_fairness_assert:
    assert property (cache_0_strong_fairness);
cache_1_strong_fairness_assert:
    assert property (cache_1_strong_fairness);
cache_0_justice_cover:
    cover property (cache_0_justice);
cache_1_justice_cover:
    cover property (cache_1_justice);
cache_0_strong_fairness_cover:
    cover property (cache_0_strong_fairness);
cache_1_strong_fairness_cover:
    cover property (cache_1_strong_fairness);
directory_both_clients_accepted_cover:
    cover property (directory_both_clients_accepted);
