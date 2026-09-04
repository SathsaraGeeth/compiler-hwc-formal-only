property reset_state;
    @(posedge clk)
    disable iff (!f_past_valid)
    !rst_n |=> ((dut.s_tx_state == '0) &&
                (dut.s_rx_state == '0) && tx &&
                tx_empty && rx_empty &&
                (tx_level == '0) && (rx_level == '0));
endproperty

assert property (reset_state);
