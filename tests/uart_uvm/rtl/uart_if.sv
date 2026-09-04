/*
 * uart_if.sv
 *
 * Copyright (C) 2024 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | 64-bit MMIO implementation
 */

`timescale 1ns/1ps
`default_nettype none

interface uart_if #(
    parameter RX_DEPTH          = 8,
    parameter TX_DEPTH          = 8,
    parameter PARITY_EN         = 0,
    parameter EN_2STOP_BITS     = 0
);
    logic                           clk;
    logic                           rst_n;

    logic  [31:0]                   baud_div;
    logic  [7:0]                    deq_rx_data;
    logic                           deq_rx_ready;
    logic                           deq_rx_valid;

    logic                           rx_full;
    logic                           rx_empty;
    logic [$clog2(RX_DEPTH+1)-1:0]  rx_level;

    logic  [7:0]                    enq_tx_data;
    logic                           enq_tx_valid;
    logic                           enq_tx_ready;

    logic                           tx_full;
    logic                           tx_empty;
    logic [$clog2(TX_DEPTH+1)-1:0]  tx_level;

    logic                           rx;
    logic                           tx;

endinterface: uart_if
