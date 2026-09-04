/*
 * arbiter.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. Round robin arbiter (for simiplicity) for memory requests
 */

import interface_pkg::*;

module memory_arbiter #(
    parameter int NUM_PORTS = 4
) (
    input  logic       i_clk,
    input  logic       i_rst_n,

    input  mem_req_t   i_arb_req        [NUM_PORTS-1:0],
    input  logic       i_arb_req_valid  [NUM_PORTS-1:0],
    output logic       o_arb_req_ready  [NUM_PORTS-1:0],
    output mem_rsp_t   o_arb_rsp        [NUM_PORTS-1:0],
    output logic       o_arb_rsp_valid  [NUM_PORTS-1:0],
    input  logic       i_arb_rsp_ready  [NUM_PORTS-1:0],

    output mem_req_t   o_mem_req,
    output logic       o_mem_req_valid,
    input  logic       i_mem_req_ready,
    input  mem_rsp_t   i_mem_rsp,
    input  logic       i_mem_rsp_valid,
    output logic       o_mem_rsp_ready
);
    localparam int PORT_W = (NUM_PORTS > 1) ? $clog2(NUM_PORTS) : 1;

    /* round robin (choose a winner for req)---  */
    logic [PORT_W-1:0] r_last_winner;
    logic [PORT_W-1:0] w_this_winner;
    logic              w_winner_valid;

    always_comb begin
        w_this_winner  = '0;
        w_winner_valid = 1'b0;

        for (int offset = 1; offset <= NUM_PORTS; offset++) begin
            if (!w_winner_valid && 
                 i_arb_req_valid[(r_last_winner + offset) % NUM_PORTS]) begin

                w_this_winner  = PORT_W'((r_last_winner + offset) % NUM_PORTS);
                w_winner_valid = 1'b1;
            end
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            r_last_winner <= PORT_W'(NUM_PORTS-1);
        end else if (w_winner_valid && o_arb_req_ready[w_this_winner]) begin
            r_last_winner <= w_this_winner;
        end
    end
    /*  ---round robin */


    /* arb_req_to_mem_req---  */
    always_comb begin
        o_arb_req_ready = '{default: 1'b0};

        for (int port_id = 0; port_id < NUM_PORTS; port_id++) begin
            o_arb_req_ready[port_id] = w_winner_valid &&
                (w_this_winner == port_id[PORT_W-1:0]) &&
                (!o_mem_req_valid || i_mem_req_ready);
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            o_mem_req       <= '0;
            o_mem_req_valid <= 1'b0;
        end else if (!o_mem_req_valid || i_mem_req_ready) begin
            o_mem_req_valid <= w_winner_valid;
            if (w_winner_valid) begin
                o_mem_req              <= i_arb_req[w_this_winner];
                o_mem_req.requester_id <= MEM_REQ_ID_W'(w_this_winner);
            end
        end
    end
    /* ---arb_req_to_mem_req */



    /* mem_rsp_to_arb_rsp---  */
    always_comb begin
        o_mem_rsp_ready = 1'b0;
        for (int port_id = 0; port_id < NUM_PORTS; port_id++) begin
            if (i_mem_rsp_valid &&
                (i_mem_rsp.requester_id == port_id[MEM_REQ_ID_W-1:0])) begin
                o_mem_rsp_ready = !o_arb_rsp_valid[port_id] ||
                                  i_arb_rsp_ready[port_id];
            end
        end
    end

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            for (int port_id = 0; port_id < NUM_PORTS; port_id++) begin
                o_arb_rsp[port_id]       <= '0;
                o_arb_rsp_valid[port_id] <= 1'b0;
            end
        end else begin
            for (int port_id = 0; port_id < NUM_PORTS; port_id++) begin
                if (o_arb_rsp_valid[port_id] && i_arb_rsp_ready[port_id]) begin
                    o_arb_rsp_valid[port_id] <= 1'b0;
                end
            end

            if (i_mem_rsp_valid && o_mem_rsp_ready) begin
                for (int port_id = 0; port_id < NUM_PORTS; port_id++) begin
                    if (i_mem_rsp.requester_id == port_id[MEM_REQ_ID_W-1:0]) begin
                        o_arb_rsp[port_id]       <= i_mem_rsp;
                        o_arb_rsp_valid[port_id] <= 1'b1;
                    end
                end
            end
        end
    end
    /*  ---mem_rsp_to_arb_rsp */

endmodule: memory_arbiter
