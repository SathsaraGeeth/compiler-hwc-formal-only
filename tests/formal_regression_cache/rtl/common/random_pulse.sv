/*
 * random.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. A psuedo random pulse generator for modeling purposes
 */

module random_pulse (
    input  logic i_clk,
    input  logic i_rst_n,
    output logic o_pulse
);
    logic [7:0] lfsr;

    always_ff @(posedge i_clk) begin
        if (!i_rst_n) begin
            lfsr <= 8'h1;
        end
        else if (lfsr == 8'h00) begin
            lfsr <= 8'h01;
        end
        else begin
            lfsr <= {lfsr[6:0],
            lfsr[7]^lfsr[5]^lfsr[4]^lfsr[3]};
        end
    end

    assign o_pulse = lfsr[7];

endmodule: random_pulse
