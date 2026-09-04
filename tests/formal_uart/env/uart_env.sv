`timescale 1ns/1ps
`default_nettype none

module uart_formal (
    input logic        clk,
    input logic        rst_n,
    input logic [31:0] baud_div,
    input logic        rx,
    input logic [7:0]  enq_tx_data,
    input logic        enq_tx_valid,
    input logic        deq_rx_valid
);
    localparam int         RX_DEPTH          = 2;
    localparam int         TX_DEPTH          = 2;
    localparam logic [3:0] F_TX_IDLE         = 4'd0;
    localparam logic [3:0] F_TX_START        = 4'd1;
    localparam logic [3:0] F_TX_D0           = 4'd2;
    localparam logic [3:0] F_TX_D7           = 4'd9;
    localparam logic [3:0] F_TX_STOP0        = 4'd11;
    localparam logic [3:0] F_RX_START_DETECT = 4'd1;
    localparam logic [3:0] F_RX_D0           = 4'd2;

    logic [7:0]                    deq_rx_data;
    logic                          deq_rx_ready;
    logic                          rx_full;
    logic                          rx_empty;
    logic [$clog2(RX_DEPTH+1)-1:0] rx_level;
    logic                          enq_tx_ready;
    logic                          tx_full;
    logic                          tx_empty;
    logic [$clog2(TX_DEPTH+1)-1:0] tx_level;
    logic                          tx;

    uart #(
        .RX_DEPTH      (RX_DEPTH),
        .TX_DEPTH      (TX_DEPTH),
        .PARITY_EN     (0),
        .EN_2STOP_BITS (0)
    ) dut (
        .i_clk          (clk),
        .i_rst_n        (rst_n),
        .i_baud_div     (baud_div),
        .o_deq_rx_data  (deq_rx_data),
        .o_deq_rx_ready (deq_rx_ready),
        .i_deq_rx_valid (deq_rx_valid),
        .o_rx_full      (rx_full),
        .o_rx_empty     (rx_empty),
        .o_rx_level     (rx_level),
        .i_enq_tx_data  (enq_tx_data),
        .i_enq_tx_valid (enq_tx_valid),
        .o_enq_tx_ready (enq_tx_ready),
        .o_tx_full      (tx_full),
        .o_tx_empty     (tx_empty),
        .o_tx_level     (tx_level),
        .i_rx           (rx),
        .o_tx           (tx)
    );

    logic f_past_valid = 1'b0;

    always_ff @(posedge clk)
        f_past_valid <= 1'b1;

    `include "uart_assumptions.sv"
    `include "uart_reset.sv"
    `include "uart_fifo.sv"
    `include "uart_status.sv"
    `include "uart_tx.sv"
    `include "uart_rx.sv"
    `include "uart_protocol.sv"
    `include "uart_baud.sv"
    `include "uart_coverpoints.sv"
endmodule

`default_nettype wire
