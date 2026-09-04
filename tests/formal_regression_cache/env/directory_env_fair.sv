property cache_0_downstream_fair;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (s_eventually
        (f_rsp_ready_0 && f_snp_ready_0));
endproperty

property cache_1_downstream_fair;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (s_eventually
        (f_rsp_ready_1 && f_snp_ready_1));
endproperty

cache_0_downstream_fair_assume:
    assume property (cache_0_downstream_fair);
cache_1_downstream_fair_assume:
    assume property (cache_1_downstream_fair);
