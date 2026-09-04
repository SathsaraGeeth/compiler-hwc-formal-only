property fifo_ready_status;
    @(posedge clk) disable iff (!rst_n)
    (enq_tx_ready == !tx_full) &&
    (deq_rx_ready == !rx_empty);
endproperty

assert property (fifo_ready_status);
