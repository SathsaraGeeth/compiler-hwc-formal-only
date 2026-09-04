property rx_state_holds_without_tick;
    @(posedge clk) disable iff (!rst_n)
    !dut.w_samp_tick |=> $stable(dut.s_rx_state);
endproperty

property rx_serial_valid_requires_tick;
    @(posedge clk) disable iff (!rst_n)
    f_past_valid && $past(rst_n) && $changed(dut.w_s_rx_data_valid)
        |-> $past(dut.w_samp_tick);
endproperty

property rx_parallel_valid_requires_tick;
    @(posedge clk) disable iff (!rst_n)
    f_past_valid && $past(rst_n) && $changed(dut.w_p_rx_data_valid)
        |-> $past(dut.w_samp_tick);
endproperty

assert property (rx_state_holds_without_tick);
assert property (rx_serial_valid_requires_tick);
assert property (rx_parallel_valid_requires_tick);
