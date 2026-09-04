`default_nettype none

// Property 8 cone of influence.  The directory request arbiter has two
// continuously eligible clients and alternates from the previous winner.
// This retains the arbitration state and ready/valid boundary while omitting
// directory data, response, snoop, and bus-lock datapaths.
module directory_fair_coi (
    input  logic i_clk,
    input  logic i_rst_n,
    output logic f_req_valid_0,
    output logic f_req_valid_1,
    output logic f_req_ready_0,
    output logic f_req_ready_1
);
    logic r_previous_winner;

    assign f_req_valid_0 = 1'b1;
    assign f_req_valid_1 = 1'b1;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n)
            r_previous_winner <= 1'b1;
        else
            r_previous_winner <= ~r_previous_winner;
    end

    always_comb begin
`ifndef KILL_FAIRNESS
        f_req_ready_0 = r_previous_winner;
        f_req_ready_1 = !r_previous_winner;
`else
        // Equivalent fault intent: select cache 1 every cycle.
        f_req_ready_0 = 1'b0;
        f_req_ready_1 = 1'b1;
`endif
    end

    `include "directory_fair_properties.sv"
endmodule

`default_nettype wire
