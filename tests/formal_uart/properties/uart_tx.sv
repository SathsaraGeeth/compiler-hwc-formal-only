property tx_idle_is_high;
    @(posedge clk) disable iff (!rst_n)
    (dut.s_tx_state == F_TX_IDLE) |-> tx;
endproperty

property tx_start_is_low;
    @(posedge clk) disable iff (!rst_n)
    (dut.s_tx_state == F_TX_START) |-> !tx;
endproperty

property tx_state_holds_without_tick;
    @(posedge clk) disable iff (!rst_n)
    !dut.w_baud_tick |=> $stable(dut.s_tx_state);
endproperty

assert property (tx_idle_is_high);
assert property (tx_start_is_low);
assert property (tx_state_holds_without_tick);
