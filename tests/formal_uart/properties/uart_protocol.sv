property tx_idle_transition;
    @(posedge clk) disable iff (!rst_n)
    (dut.w_baud_tick && (dut.s_tx_state == F_TX_IDLE)) |=>
    (dut.s_tx_state ==
        ($past(!tx_empty) ? F_TX_START : F_TX_IDLE));
endproperty

property tx_start_to_data0;
    @(posedge clk) disable iff (!rst_n)
    (dut.w_baud_tick && (dut.s_tx_state == F_TX_START)) |=>
    (dut.s_tx_state == F_TX_D0);
endproperty

assert property (tx_idle_transition);
assert property (tx_start_to_data0);
