/*
 * memory.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. Main memory model for simulation purposes
 * 2. Simulate 200cy latency, II=$ for reads and writes
 */

import cache_pkg::*;
import interface_pkg::*;
import coherency_pkg::*;

module main_memory #(
    parameter int NUM_LINES = 1024,
    parameter int DELAY     = 200
) (
    input  logic       i_clk,
    input  logic       i_rst_n,
    input  mem_req_t   i_mem_req,
    input  logic       i_mem_req_valid,
    output logic       o_mem_req_ready,
    output mem_rsp_t   o_mem_rsp,
    output logic       o_mem_rsp_valid,
    input  logic       i_mem_rsp_ready
);
    typedef struct packed {
        mem_req_t req;
        logic     valid;
    } payload_t;

    logic w_random_ready;

    random_pulse random_pulse_u (
        .i_clk,
        .i_rst_n,
        .o_pulse(w_random_ready)
    );

    logic [CACHE_LINE_W-1:0] mem               [0:NUM_LINES-1];
    payload_t                r_input_payload;
    payload_t                r_delayed_payload [0:DELAY-1];

    initial begin
        for (int line = 0; line < NUM_LINES; line++) begin
            mem[line] = CACHE_LINE_W'(line);
        end
    end

    assign o_mem_req_ready = w_random_ready && !r_input_payload.valid;

    always @(posedge i_clk) begin
        if (!i_rst_n) begin
            o_mem_rsp       <= '0;
            o_mem_rsp_valid <= 1'b0;
            r_input_payload <= '0;

            for (int i = 0; i < DELAY; i++) begin
                r_delayed_payload[i] <= '0;
            end

        end else begin

            if (i_mem_req_valid && o_mem_req_ready) begin
                if (i_mem_req.wen) begin
                    for (int i = 0; i < CACHE_LINE_W/8; i++)
                        if (i_mem_req.wstrb[i])
                            mem[i_mem_req.addr[BYTE_OFFSET_W +: $clog2(NUM_LINES)]][i*8 +: 8]
                                <= i_mem_req.wdata[i*8 +: 8];
                end else begin
                    r_input_payload.req   <= i_mem_req;
                    r_input_payload.valid <= 1'b1;
                end
            end

            if (!o_mem_rsp_valid || i_mem_rsp_ready) begin

                if (o_mem_rsp_valid && i_mem_rsp_ready) begin
                    o_mem_rsp_valid <= 1'b0;
                end

                if (r_delayed_payload[DELAY-1].valid) begin
                    if (!r_delayed_payload[DELAY-1].req.wen) begin
                        o_mem_rsp.requester_id <=
                        r_delayed_payload[DELAY-1].req.requester_id;
                        o_mem_rsp.trans_id     <= r_delayed_payload[DELAY-1].req.trans_id;
                        o_mem_rsp.rdata        <=
                        mem[r_delayed_payload[DELAY-1].req.addr[BYTE_OFFSET_W +: $clog2(NUM_LINES)]];
                        o_mem_rsp_valid        <= 1'b1;
                    end
                end

                for (int i = DELAY-1; i > 0; i--) begin
                    r_delayed_payload[i] <= r_delayed_payload[i-1];
                end

                r_delayed_payload[0] <= r_input_payload;
                if (r_input_payload.valid) begin
                    r_input_payload.valid <= 1'b0;
                end
            end
        end
    end
endmodule: main_memory
