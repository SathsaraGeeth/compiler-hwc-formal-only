property fifo_empty_flags;
    @(posedge clk) disable iff (!rst_n)
    (tx_empty == (tx_level == '0)) &&
    (rx_empty == (rx_level == '0));
endproperty

property fifo_full_flags;
    @(posedge clk) disable iff (!rst_n)
    (tx_full == (tx_level == TX_DEPTH)) &&
    (rx_full == (rx_level == RX_DEPTH));
endproperty

property fifo_levels_in_bounds;
    @(posedge clk) disable iff (!rst_n)
    (tx_level <= TX_DEPTH) && (rx_level <= RX_DEPTH);
endproperty

assert property (fifo_empty_flags);
assert property (fifo_full_flags);
assert property (fifo_levels_in_bounds);
