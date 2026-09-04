property reset_release;
    @(posedge clk)
    rst_n && tx_empty && rx_empty &&
    (dut.s_tx_state == F_TX_IDLE);
endproperty

property tx_enqueue;
    @(posedge clk)
    rst_n && enq_tx_valid && enq_tx_ready;
endproperty

property tx_start;
    @(posedge clk)
    rst_n && (dut.s_tx_state == F_TX_START);
endproperty

property tx_data0;
    @(posedge clk)
    rst_n && (dut.s_tx_state == F_TX_D0);
endproperty

property tx_data7;
    @(posedge clk)
    rst_n && (dut.s_tx_state == F_TX_D7);
endproperty

property tx_stop;
    @(posedge clk)
    rst_n && (dut.s_tx_state == F_TX_STOP0);
endproperty

property rx_start_detect;
    @(posedge clk)
    rst_n && (dut.s_rx_state == F_RX_START_DETECT);
endproperty

property rx_data0;
    @(posedge clk)
    rst_n && (dut.s_rx_state == F_RX_D0);
endproperty

property rx_byte_valid;
    @(posedge clk)
    rst_n && dut.w_p_rx_data_valid;
endproperty

property rx_dequeue;
    @(posedge clk)
    rst_n && deq_rx_valid && deq_rx_ready;
endproperty

property baud_div_change;
    @(posedge clk)
    f_past_valid && rst_n && $past(rst_n) &&
    (baud_div != $past(baud_div));
endproperty

cover property (reset_release);
cover property (tx_enqueue);
cover property (tx_start);
cover property (tx_data0);
cover property (tx_data7);
cover property (tx_stop);
cover property (rx_start_detect);
cover property (rx_data0);
cover property (rx_byte_valid);
cover property (rx_dequeue);
cover property (baud_div_change);
