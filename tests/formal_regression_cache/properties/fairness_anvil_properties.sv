property client0_service_fairness_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always s_eventually f_service0;
endproperty

property client1_service_fairness_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always s_eventually f_service1;
endproperty

client0_service_fairness_anvil_assert:
    assert property (client0_service_fairness_anvil);
client1_service_fairness_anvil_assert:
    assert property (client1_service_fairness_anvil);
