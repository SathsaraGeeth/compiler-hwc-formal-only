/*
 * uart_mmio.sv
 *
 * Copyright (C) 2026 Sathsara Geeth
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

/*
 * Comments:
 * 1. Memory Map
 *
 * Offset | Name   | Access | Description
 * -------+--------+--------+---------------------------------------------
 * 0x00   | TXDATA | RO/WO  | [63] full, [7:0] transmit data
 * 0x08   | RXDATA | RO     | [63] empty, [7:0] received data
 * 0x10   | TXCTRL | RW     | [63:1] watermark, [0] enable
 * 0x18   | RXCTRL | RW     | [63:1] watermark, [0] enable
 * 0x20   | IE     | RW     | [1] rx interrupt enable, [0] tx interrupt enable
 * 0x28   | IP     | RO     | [1] rx interrupt pending, [0] tx interrupt pending
 * 0x30   | DIV    | RW     | [63:0] baud-rate divisor
 *
 */

`timescale 1ns/1ps
`default_nettype none

module uart_mmio #(
    parameter logic [core_pkg::VIR_ADDR_W-1:0] BASE_ADDR = 64'h0000_0010_0000_0000,
    parameter int RX_DEPTH = 8,
    parameter int TX_DEPTH = 8
)(
    input  logic                            i_clk,
    input  logic                            i_rst_n,
    input  logic                            i_req_we,
    input  logic [memory_pkg::THRD_W-1:0]   i_req_thrd,
    input  logic                            i_req_is_lr,
    input  logic                            i_req_is_sc,
    input  logic [core_pkg::VIR_ADDR_W-1:0] i_req_addr,
    input  logic [core_pkg::XLEN/8-1:0]     i_req_wstrb,
    input  logic [core_pkg::XLEN-1:0]       i_req_wdata,
    input  logic                            i_req_valid,
    input  logic [isa_pkg::PRIV_MODE_W-1:0] i_req_priv,
    output logic                            o_req_ready,
    output logic [memory_pkg::THRD_W-1:0]   o_resp_thrd,
    output logic                            o_resp_sc_succ,
    output logic [core_pkg::XLEN-1:0]       o_resp_rdata,
    output logic                            o_resp_valid,
    input  logic                            i_resp_ready,
    input  logic                            i_rx,
    output logic                            o_tx
);
    localparam int XLEN       = core_pkg::XLEN;
    localparam int VIR_ADDR_W = core_pkg::VIR_ADDR_W;

    typedef enum logic [2:0] {
        TXDATA,
        RXDATA,
        TXCTRL,
        RXCTRL,
        IE,
        IP,
        DIV,
        ILLEGAL
    } reg_t;

    function automatic reg_t decode_reg(
        input logic [VIR_ADDR_W-1:0] addr
    );
        case (addr - BASE_ADDR)
            VIR_ADDR_W'(64'h00): return TXDATA;
            VIR_ADDR_W'(64'h08): return RXDATA;
            VIR_ADDR_W'(64'h10): return TXCTRL;
            VIR_ADDR_W'(64'h18): return RXCTRL;
            VIR_ADDR_W'(64'h20): return IE;
            VIR_ADDR_W'(64'h28): return IP;
            VIR_ADDR_W'(64'h30): return DIV;
            default:             return ILLEGAL;
        endcase
    endfunction: decode_reg

    function automatic logic [XLEN-1:0] apply_wmask(
        input logic [XLEN-1:0]   old_value,
        input logic [XLEN-1:0]   new_value,
        input logic [XLEN/8-1:0] byte_mask
    );
        logic [XLEN-1:0] value;

        value = old_value;
        for (int byte_idx = 0; byte_idx < XLEN/8; byte_idx++) begin
            if (byte_mask[byte_idx]) begin
                value[byte_idx*8 +: 8] = new_value[byte_idx*8 +: 8];
            end
        end
        return value;
    endfunction: apply_wmask

    /* UART signals---  */
    logic [7:0]                         w_tx_data;
    logic [7:0]                         w_rx_data;
    logic                               w_rx_empty;
    logic                               w_tx_valid;
    logic                               w_tx_ready;
    logic                               w_rx_valid;
    logic                               w_tx_full;
    logic [31:0]                        w_div;
    logic [$clog2(TX_DEPTH+1)-1:0]      w_tx_level;
    logic [$clog2(RX_DEPTH+1)-1:0]      w_rx_level;
    /*  ---UART signals */

    /* MMIO regs---  */
    logic [XLEN-1:0] r_txctrl;
    logic [XLEN-1:0] r_rxctrl;
    logic [XLEN-1:0] r_ie;
    logic [XLEN-1:0] r_div;
    /*  ---MMIO regs */

    /* registered request---  */
    reg_t                            r_req_type;
    logic                            r_req_we;
    logic [memory_pkg::THRD_W-1:0]   r_req_thrd;
    logic [XLEN-1:0]                 r_req_wdata;
    logic [XLEN/8-1:0]               r_req_wstrb;
    logic [isa_pkg::PRIV_MODE_W-1:0] r_req_priv;
    logic                            r_req_valid;
    logic                            w_req_complete;

    always_ff @(posedge i_clk) begin: register_request
        if (!i_rst_n) begin
            r_req_type  <= ILLEGAL;
            r_req_we    <= 1'b0;
            r_req_thrd  <= '0;
            r_req_wdata <= '0;
            r_req_wstrb <= '0;
            r_req_priv  <= isa_pkg::PRIV_MODE_M;
            r_req_valid <= 1'b0;
        end else if (i_req_valid && o_req_ready) begin
            r_req_type  <= decode_reg(i_req_addr);
            r_req_we    <= i_req_we;
            r_req_thrd  <= i_req_thrd;
            r_req_wdata <= i_req_wdata;
            r_req_wstrb <= i_req_wstrb;
            r_req_priv  <= i_req_priv;
            r_req_valid <= 1'b1;
        end else if (r_req_valid && w_req_complete) begin
            r_req_valid <= 1'b0;
        end
    end: register_request
    /*  ---registered request */

    /* MMIO writes---  */
    always_ff @(posedge i_clk) begin: mmio_write
        if (!i_rst_n) begin
            r_txctrl <= '0;
            r_rxctrl <= '0;
            r_ie     <= '0;
            r_div    <= XLEN'(32'd1085);
        end else if (r_req_valid && r_req_we && (r_req_priv != isa_pkg::PRIV_MODE_U)) begin
            unique case (r_req_type)
                TXCTRL: r_txctrl <= apply_wmask(r_txctrl, r_req_wdata, r_req_wstrb);
                RXCTRL: r_rxctrl <= apply_wmask(r_rxctrl, r_req_wdata, r_req_wstrb);
                IE:     r_ie     <= apply_wmask(r_ie, r_req_wdata, r_req_wstrb) & XLEN'(64'h3);
                DIV:    r_div    <= apply_wmask(r_div, r_req_wdata, r_req_wstrb);
                default: begin
                end
            endcase
        end
    end: mmio_write
    /*  ---MMIO writes */

    /* MMIO response---  */
    logic r_resp_busy;
    logic [XLEN-1:0] w_resp_rdata;

    always_comb begin: mmio_read_mux
        unique case (r_req_type)
            TXDATA:  w_resp_rdata = {w_tx_full, 55'b0, 8'b0};
            RXDATA:  w_resp_rdata = {w_rx_empty, 55'b0, w_rx_data};
            TXCTRL:  w_resp_rdata = r_txctrl;
            RXCTRL:  w_resp_rdata = r_rxctrl;
            IE:      w_resp_rdata = r_ie;
            IP:      w_resp_rdata = {{XLEN-2{1'b0}}, 63'(w_rx_level) >= r_rxctrl[63:1], 63'(w_tx_level) <= r_txctrl[63:1]};
            DIV:     w_resp_rdata = r_div;
            default: w_resp_rdata = '0;
        endcase
    end: mmio_read_mux

    always_ff @(posedge i_clk) begin: mmio_response
        if (!i_rst_n) begin
            r_resp_busy    <= 1'b0;
            o_resp_thrd    <= '0;
            o_resp_sc_succ <= 1'b0;
            o_resp_rdata   <= '0;
            o_resp_valid   <= 1'b0;
        end else begin
            if (r_req_valid && w_req_complete) begin
                r_resp_busy    <= 1'b1;
                o_resp_thrd    <= r_req_thrd;
                o_resp_sc_succ <= 1'b0;
                o_resp_rdata   <= w_resp_rdata;
                o_resp_valid   <= 1'b1;
            end

            if (o_resp_valid && i_resp_ready) begin
                r_resp_busy  <= 1'b0;
                o_resp_valid <= 1'b0;
            end
        end
    end: mmio_response

    assign o_req_ready = !r_resp_busy && !r_req_valid;
    /*  ---MMIO response */

    /* UART datapath---  */
    assign w_tx_data      = r_req_wdata[7:0];
    assign w_tx_valid     = r_req_valid && r_req_we && (r_req_type == TXDATA) && r_req_wstrb[0] &&
                           (r_req_priv != isa_pkg::PRIV_MODE_U) && r_txctrl[0];
    assign w_req_complete = !w_tx_valid || w_tx_ready;
    assign w_rx_valid     = r_req_valid && !r_req_we && (r_req_type == RXDATA) &&
                           (r_req_priv != isa_pkg::PRIV_MODE_U) && r_rxctrl[0];
    assign w_div          = r_div[31:0];
    /*  ---UART datapath */

    uart #(
        .RX_DEPTH       (RX_DEPTH),
        .TX_DEPTH       (TX_DEPTH),
        .PARITY_EN      (0),
        .EN_2STOP_BITS  (0)
    ) uart_u (
        .i_clk          (i_clk),
        .i_rst_n        (i_rst_n),
        .i_baud_div     (w_div),
        .o_deq_rx_data  (w_rx_data),
        .o_deq_rx_ready (),
        .i_deq_rx_valid (w_rx_valid),
        .o_rx_full      (),
        .o_rx_empty     (w_rx_empty),
        .o_rx_level     (w_rx_level),
        .i_enq_tx_data  (w_tx_data),
        .i_enq_tx_valid (w_tx_valid),
        .o_enq_tx_ready (w_tx_ready),
        .o_tx_full      (w_tx_full),
        .o_tx_empty     (),
        .o_tx_level     (w_tx_level),
        .i_rx           (i_rx),
        .o_tx           (o_tx)
    );

endmodule: uart_mmio

`default_nettype wire
