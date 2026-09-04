property baud_tick_matches_divisor;
    @(posedge clk) disable iff (!rst_n)
    !((dut.s_tx_state == F_TX_IDLE) &&
      (dut.s_rx_state == 4'd0) &&
      (dut.r_baud_div != dut.r_baud_div_pend)) &&
    (dut.r_baud_ctr == (dut.r_baud_div - 1'b1))
        |=> dut.w_baud_tick && (dut.r_baud_ctr == 0);
endproperty

property sample_tick_matches_divisor;
    @(posedge clk) disable iff (!rst_n)
    !((dut.s_tx_state == F_TX_IDLE) &&
      (dut.s_rx_state == 4'd0) &&
      (dut.r_baud_div != dut.r_baud_div_pend)) &&
    (dut.r_samp_ctr == ((dut.r_baud_div / 16) - 1'b1))
        |=> dut.w_samp_tick && (dut.r_samp_ctr == 0);
endproperty

assert property (baud_tick_matches_divisor);
assert property (sample_tick_matches_divisor);
