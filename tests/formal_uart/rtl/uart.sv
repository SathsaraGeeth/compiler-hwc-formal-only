/*
 * uart.sv
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

// 8N1/8N2 UART
module uart #(
    parameter RX_DEPTH          = 8,
    parameter TX_DEPTH          = 8,
    parameter PARITY_EN         = 0,        // [0] not implemented; as it is very uncommon in practice - see [4], [5], [6]
    parameter EN_2STOP_BITS     = 0

    ) ( 
    input   logic                           i_clk,
    input   logic                           i_rst_n,
    input   logic  [31:0]                   i_baud_div,  // [1] width may be reduced - see [2]. ideally baud_div = 16*n, where n is an integer no less than 1 - see [3]
    output  logic  [7:0]                    o_deq_rx_data,
    output  logic                           o_deq_rx_ready,
    input   logic                           i_deq_rx_valid,
    output  logic                           o_rx_full,
    output  logic                           o_rx_empty,
    output  logic [$clog2(RX_DEPTH+1)-1:0]  o_rx_level,
    input   logic  [7:0]                    i_enq_tx_data,
    input   logic                           i_enq_tx_valid,
    output  logic                           o_enq_tx_ready,
    output  logic                           o_tx_full,
    output  logic                           o_tx_empty,
    output  logic [$clog2(TX_DEPTH+1)-1:0]  o_tx_level,
    input   logic                           i_rx,
    output  logic                           o_tx
);  

    // 1. Pulses
    logic [31:0] r_baud_div;                // [2] width may be reduced
    logic [31:0] r_baud_div_pend;           // [2] width may be reduced
    logic [31:0] r_baud_ctr;                // [2] width may be reduced
    logic [31:0] r_samp_ctr;                // [2] width may be reduced
    logic        w_baud_tick;
    logic        w_samp_tick;
    logic        w_div_update_safe;

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_baud_div_pend <= 32'd16;
            r_baud_div      <= 32'd16;
        end else begin
            r_baud_div_pend <= (i_baud_div < 32'd16) ? 32'd16 : i_baud_div;
            if (w_div_update_safe) begin
                r_baud_div <= r_baud_div_pend;
            end
        end
    end

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_baud_ctr          <= '0;
            r_samp_ctr          <= '0;
            w_baud_tick         <= 1'b0;
            w_samp_tick         <= 1'b0;
        end else if (w_div_update_safe && (r_baud_div != r_baud_div_pend)) begin
            r_baud_ctr          <= '0;
            r_samp_ctr          <= '0;
            w_baud_tick         <= 1'b0;
            w_samp_tick         <= 1'b0;
        end else begin
            if (r_baud_ctr == r_baud_div-1) begin
                r_baud_ctr      <= 0;
                w_baud_tick     <= 1'b1;
            end else begin
                r_baud_ctr      <= r_baud_ctr + 1;
                w_baud_tick     <= 1'b0;
            end
            if (r_samp_ctr == r_baud_div/16-1) begin    // [3] - we accept the jitter if div!=16n
                r_samp_ctr      <= 0;
                w_samp_tick     <= 1'b1;
            end else begin
                r_samp_ctr      <= r_samp_ctr + 1;
                w_samp_tick     <= 1'b0;
            end
        end
    end

    // pulses

    // 2. TX
    typedef enum logic [$clog2(13)-1:0] { 
        TX_IDLE,
        TX_START,
        TX_D0,
        TX_D1,
        TX_D2,
        TX_D3,
        TX_D4,
        TX_D5,
        TX_D6,
        TX_D7,
        TX_PARITY,
        TX_STOP0,
        TX_STOP1
    }   tx_state_t;

    tx_state_t   s_tx_state, s_tx_next;

    always_ff  @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            s_tx_state  <= TX_IDLE;
        end else if (w_baud_tick) begin
            s_tx_state  <= s_tx_next;
        end
    end

    logic           w_valid_tx;
    logic           w_en_txshftr;
    logic           w_load_txshftr;
    logic   [7:0]   w_p_tx_data;
    logic           w_s_txsfhtr_data;

    fifo #(.DEPTH(TX_DEPTH), .WIDTH(8)) tx_buffer (
        .i_clk(i_clk),
        .i_rst_n(i_rst_n),

        .o_head_data(w_p_tx_data),
        .o_deq_ready(w_valid_tx),
        .i_deq_valid(w_load_txshftr),

        .i_enq_tail_data(i_enq_tx_data),
        .i_enq_valid(i_enq_tx_valid),
        .o_enq_ready(o_enq_tx_ready),

        .o_full(o_tx_full),
        .o_empty(o_tx_empty),
        .o_level(o_tx_level)
    );

    p2s_shift_reg   #(.WIDTH(8)) r_tx_shft_reg (
        .i_clk(i_clk),
        .i_rst_n(i_rst_n),

        .i_en(w_en_txshftr),
        .i_p_data(w_p_tx_data),
        .i_load(w_load_txshftr),
        .o_s_data(w_s_txsfhtr_data)
    );

    always_comb begin
        unique case (s_tx_state)
            TX_IDLE:   begin
                    o_tx            = 1'b1;
                    w_load_txshftr  = w_valid_tx && w_baud_tick;
                    w_en_txshftr    = 1'b0;
                    s_tx_next       = w_valid_tx ? TX_START : TX_IDLE;
            end
            TX_START:  begin
                    o_tx            = 1'b0;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = 1'b0;
                    s_tx_next       = TX_D0;
            end
            TX_D0: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D1;
            end
            TX_D1: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D2;
            end
            TX_D2: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D3;
            end
            TX_D3: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D4;
            end
            TX_D4: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D5;
            end
            TX_D5: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D6;
            end
            TX_D6: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = TX_D7;
            end
            TX_D7: begin
                    o_tx            = w_s_txsfhtr_data;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = w_baud_tick;
                    s_tx_next       = PARITY_EN ? TX_PARITY : TX_STOP0;
            end
            TX_PARITY: begin
                    o_tx            = 1'b1;         // [4] Set this value based on the w_p_tx_data and the parity scheme if parity is required.
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = 1'b0;
                    s_tx_next       = TX_STOP0;
            end 
            TX_STOP0:  begin
                    o_tx            = 1'b1;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = 1'b0;
                    s_tx_next       = EN_2STOP_BITS   ? TX_STOP1 : TX_IDLE;
            end     
            TX_STOP1:  begin
                    o_tx            = 1'b1;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = 1'b0;
                    s_tx_next       = TX_IDLE;
            end
            default: begin
                    o_tx            = 1'b1;
                    w_load_txshftr  = 1'b0;
                    w_en_txshftr    = 1'b0;
                    s_tx_next       = TX_IDLE;
`ifndef FORMAL
                    if (i_rst_n && s_tx_state !== 'x) begin
                        $error("Illegal TX state!");
                    end
`endif
            end
        endcase
    end
    // TX - Trasmitter

    // 3. RX - Reciever
    typedef enum logic [$clog2(13)-1:0] { 
        RX_IDLE,
        RX_START_DETECT,
        RX_D0,
        RX_D1,
        RX_D2,
        RX_D3,
        RX_D4,
        RX_D5,
        RX_D6,
        RX_D7,
        RX_PARITY,
        RX_STOP0,
        RX_STOP1
    }   rx_state_t;

    rx_state_t   s_rx_state;

    logic [4:0]     samp_ctr;
    logic [1:0]     ones_ctr;

    logic           w_s_rx_data;
    logic           w_s_rx_data_valid;
    logic   [7:0]   w_p_rx_data;
    logic           w_p_rx_data_valid;


    // logic   w_s_rx_data_valid_pulse;
    // level2pulse l2p_1(
    //     .i_clk(i_clk),
    //     .i_rst_n(i_rst_n),
    //     .i_level(w_s_rx_data_valid),
    //     .o_pulse(w_s_rx_data_valid_pulse)
    // );

    s2p_shift_reg #(.WIDTH(8)) r_rx_shft_reg (
        .i_clk(i_clk),
        .i_rst_n(i_rst_n),
        .i_en(w_s_rx_data_valid/*w_s_rx_data_valid_pulse*/),
        .i_s_data(w_s_rx_data),
        .o_p_data(w_p_rx_data)
    );

    logic   w_sync_rx;

    sync_2ff sync0 (
        .i_clk(i_clk),
        .i_rst_n(i_rst_n),
        .i_async(i_rx),
        .o_sync(w_sync_rx)
    );

    assign w_div_update_safe = (s_tx_state == TX_IDLE) && (s_rx_state == RX_IDLE) && w_sync_rx;
    assign w_s_rx_data       = (ones_ctr < 2) ? 1'b0 : 1'b1;

    always_ff  @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            s_rx_state  <= RX_IDLE;
            samp_ctr    <= 0;
            ones_ctr    <= 0;
            w_s_rx_data_valid <= 1'b0;
            w_p_rx_data_valid <= 1'b0;
        end else begin
            w_s_rx_data_valid <= 1'b0;
            w_p_rx_data_valid <= 1'b0;
            if (w_samp_tick) begin
                samp_ctr <= samp_ctr + 1;
                unique case (s_rx_state)
                RX_IDLE:    begin
                    s_rx_state  <= (w_sync_rx == 1'b0) ? RX_START_DETECT : RX_IDLE;
                    ones_ctr    <= 0;
                    samp_ctr    <= 1;
                end
                RX_START_DETECT: begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= (ones_ctr < 2) ? RX_D0 : RX_IDLE;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D0:
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D1;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D1: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D2;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D2: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1; 
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D3;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D3: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D4;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D4: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D5;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D5: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D6;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D6: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_D7;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_D7: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_s_rx_data_valid <= 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= PARITY_EN ? RX_PARITY : RX_STOP0;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_PARITY: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        // [5] If parity is needed check parity here based on the previous values and the scheme as well.
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= PARITY_EN ? RX_PARITY : RX_STOP0;    //  [6] If parity failed, change this so that it direclty drops it.
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_STOP0: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_p_rx_data_valid <= EN_2STOP_BITS ? 1'b0 : (ones_ctr < 2) ? 1'b0 : 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= (EN_2STOP_BITS && ~(ones_ctr < 2)) ? RX_STOP1 : RX_IDLE;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                RX_STOP1: 
                begin
                    if ((samp_ctr == 7) || (samp_ctr == 8) || (samp_ctr == 9)) begin
                        ones_ctr <= ones_ctr + w_sync_rx;
                    end else if (samp_ctr == 10) begin
                        w_p_rx_data_valid <= (ones_ctr < 2) ? 1'b0 : 1'b1;
                    end else if (samp_ctr == 16) begin
                        s_rx_state  <= RX_IDLE;
                        samp_ctr    <= 1;
                        ones_ctr    <= 0;
                    end
                end
                default: begin
                        w_s_rx_data_valid <= 1'b0;
                        w_p_rx_data_valid <= 1'b0;
`ifndef FORMAL
                        $error("Illegal RX state!");
`endif
                end
                endcase
            end
        end
    end

    // logic   w_p_rx_data_valid_pulse;
    // level2pulse l2p_2(
    //     .i_clk(i_clk),
    //     .i_rst_n(i_rst_n),
    //     .i_level(w_p_rx_data_valid),
    //     .o_pulse(w_p_rx_data_valid_pulse)
    // );

    fifo #(.DEPTH(RX_DEPTH), .WIDTH(8))
        rx_buffer   (
        .i_clk(i_clk),
        .i_rst_n(i_rst_n),

        .o_head_data(o_deq_rx_data),
        .o_deq_ready(o_deq_rx_ready),
        .i_deq_valid(i_deq_rx_valid),

        .i_enq_tail_data(w_p_rx_data),
        .i_enq_valid(w_p_rx_data_valid/*w_p_rx_data_valid_pulse*/),
        .o_enq_ready(),                         // [7] drops if can't accept

        .o_full(o_rx_full),
        .o_empty(o_rx_empty),
        .o_level(o_rx_level)
    );
    // RX - Reciever

endmodule: uart // universal asyncronous reciever and transmitter


module fifo #(
    parameter  DEPTH = 8,
    parameter  WIDTH = 8
) (
    input  logic                           i_clk,
    input  logic                           i_rst_n,
    output logic   [WIDTH-1:0]             o_head_data,
    output logic                           o_deq_ready,
    input  logic                           i_deq_valid,
    input  logic   [WIDTH-1:0]             i_enq_tail_data,
    input  logic                           i_enq_valid,
    output logic                           o_enq_ready,
    output logic                           o_full,
    output logic                           o_empty,
    output logic   [$clog2(DEPTH+1)-1:0]   o_level
);  
    logic   [WIDTH-1:0]                 mem_buff    [0:DEPTH-1];
    logic   [$clog2(DEPTH)-1:0]         r_head_ptr;
    logic   [$clog2(DEPTH)-1:0]         r_tail_ptr;
    logic   [$clog2(DEPTH+1)-1:0]       r_count;

    always_comb begin
        o_full              = (r_count == DEPTH);
        o_empty             = (r_count == '0);
        o_level             = r_count;
        o_deq_ready         = !o_empty;
        o_enq_ready         = !o_full;
        o_head_data         = mem_buff[r_head_ptr];
    end

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_head_ptr  <= '0;
            r_tail_ptr  <= '0;
            r_count     <= '0;
        end else begin
            case ({o_enq_ready & i_enq_valid, o_deq_ready & i_deq_valid})
                2'b01: begin
                    r_head_ptr  <= r_head_ptr + 1;
                    r_count     <= r_count    - 1;
                end
                2'b10: begin
                    r_tail_ptr           <= r_tail_ptr + 1;
                    r_count              <= r_count    + 1;
                    mem_buff[r_tail_ptr] <= i_enq_tail_data;
                end
                2'b11:  begin
                    r_head_ptr           <= r_head_ptr + 1;
                    r_tail_ptr           <= r_tail_ptr + 1;
                    mem_buff[r_tail_ptr] <= i_enq_tail_data;
                end
                default: begin end
            endcase
        end
    end
endmodule: fifo

module p2s_shift_reg #(parameter WIDTH = 8) (
    input               i_clk,
    input               i_rst_n,
    input               i_en,
    input  [WIDTH-1:0]  i_p_data,
    input               i_load,
    output              o_s_data
);
    logic [WIDTH-1:0]   r_shft_reg;

    assign o_s_data = r_shft_reg[0];

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_shft_reg <= '0;
        end else if (i_load) begin
            r_shft_reg <= i_p_data;
        end else if (i_en) begin
            r_shft_reg <= {1'b0, r_shft_reg[WIDTH-1:1]};
        end
    end
endmodule: p2s_shift_reg   // parallel to serial shift register

module s2p_shift_reg #(parameter WIDTH = 8) (
    input               i_clk,
    input               i_rst_n,
    input               i_en,
    input               i_s_data,
    output [WIDTH-1:0]  o_p_data
);
    logic   [WIDTH-1:0]     r_shft_reg;

    assign o_p_data = r_shft_reg;

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_shft_reg <= '0;
        end else if (i_en) begin
            r_shft_reg <= {i_s_data, r_shft_reg[WIDTH-1:1]};
        end
    end
endmodule: s2p_shift_reg   // serial to parallel shift register

module level2pulse (
    input   i_clk,
    input   i_rst_n,
    input   i_level,
    output  o_pulse
);
    logic   r_level;
    assign  o_pulse = ~r_level & i_level;

    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_level <= 1'b0;
        end else begin
            r_level <= i_level;
        end
    end
endmodule: level2pulse // convert a level to a pulse

module sync_2ff (
    input  logic i_clk,
    input  logic i_rst_n,
    input  logic i_async,
    output logic o_sync
);
    logic r_ff1, r_ff2;
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_ff1 <= 1'b0;
            r_ff2 <= 1'b0;
        end else begin
            r_ff1 <= i_async;
            r_ff2 <= r_ff1;
        end
    end
    assign o_sync = r_ff2;
endmodule: sync_2ff // sync and async to the clk using 2 ff's
