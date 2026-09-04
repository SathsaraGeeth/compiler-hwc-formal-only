`default_nettype none

import core_pkg::*;
import cache_pkg::*;
import coherency_pkg::*;

module directory_reachability (
    input  logic     i_clk,
    input  logic     i_rst_n,
    input  logic     f_cache_req_valid_0,
    input  logic     f_cache_req_valid_1,
    input  coh_req_e f_cache_req_msg_0,
    input  coh_req_e f_cache_req_msg_1,
    input  logic     f_request_needs_snoop,
    input  logic     f_snoop_ready,
    input  logic     f_matching_ack_valid,
    input  logic     f_response_ready,
    input  logic     f_lock_req_valid,
    input  logic     f_lock_response_ready,
    output logic     f_direct_grant_reached,
    output logic     f_snoop_ack_grant_reached,
    output logic     f_both_clients_accepted,
    output logic     f_bus_lock_response_reached
);
    logic cache_pipeline_busy;
    logic snoop_waiting_for_ack;
    logic snoop_grant_pending;
    logic client_0_accepted;
    logic client_1_accepted;
    logic lock_pending;

    logic accept_0;
    logic accept_1;
    logic accepted_get;
    logic accepted_direct_get;
    logic accepted_snoop_get;

    assign accept_0 = f_cache_req_valid_0 && !cache_pipeline_busy;
    assign accept_1 = f_cache_req_valid_1 && !cache_pipeline_busy && !accept_0;
    assign accepted_get = (accept_0 &&
                           ((f_cache_req_msg_0 == GETS) ||
                            (f_cache_req_msg_0 == GETM))) ||
                          (accept_1 &&
                           ((f_cache_req_msg_1 == GETS) ||
                            (f_cache_req_msg_1 == GETM)));
    assign accepted_direct_get = accepted_get && !f_request_needs_snoop;
    assign accepted_snoop_get  = accepted_get && f_request_needs_snoop;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            cache_pipeline_busy        <= 1'b0;
            snoop_waiting_for_ack       <= 1'b0;
            snoop_grant_pending         <= 1'b0;
            client_0_accepted           <= 1'b0;
            client_1_accepted           <= 1'b0;
            lock_pending                <= 1'b0;
            f_direct_grant_reached      <= 1'b0;
            f_snoop_ack_grant_reached   <= 1'b0;
            f_both_clients_accepted     <= 1'b0;
            f_bus_lock_response_reached <= 1'b0;
        end else begin
            if (accept_0) client_0_accepted <= 1'b1;
            if (accept_1) client_1_accepted <= 1'b1;
            if ((client_0_accepted || accept_0) &&
                (client_1_accepted || accept_1)) begin
                f_both_clients_accepted <= 1'b1;
            end

            if (accepted_direct_get) begin
                cache_pipeline_busy <= 1'b1;
            end
            if (cache_pipeline_busy && f_response_ready) begin
                cache_pipeline_busy   <= 1'b0;
                f_direct_grant_reached <= 1'b1;
            end

            if (accepted_snoop_get && f_snoop_ready) begin
                snoop_waiting_for_ack <= 1'b1;
            end
            if (snoop_waiting_for_ack && f_matching_ack_valid) begin
                snoop_waiting_for_ack <= 1'b0;
                snoop_grant_pending   <= 1'b1;
            end
            if (snoop_grant_pending && f_response_ready) begin
                snoop_grant_pending       <= 1'b0;
                f_snoop_ack_grant_reached <= 1'b1;
            end

            if (f_lock_req_valid && !lock_pending) begin
                lock_pending <= 1'b1;
            end
            if (lock_pending && f_lock_response_ready) begin
                lock_pending                <= 1'b0;
                f_bus_lock_response_reached <= 1'b1;
            end
        end
    end

    `include "directory_env_reachability.sv"
    `include "directory_reachability_properties.sv"
endmodule

`default_nettype wire
