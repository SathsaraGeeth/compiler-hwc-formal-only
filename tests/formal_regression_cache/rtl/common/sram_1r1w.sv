/*
 * sram_1r1w.sv
 *
 * 2026
 */

/*
 * Comments:
 * 1. Reusable SRAM bank
 * 2. 1r1w SRAM
 * 3. 1cy read and 1cy write latency
 * 4. Intiate Interval(II) = 1cy
 * 5. Read and write during same cycle
 *    behavior is undefined. Don't rely on it
 * 6. After reset behavior is undefined
 * 7. Pipe char: stiff 1cy latency, II=1cy
 */

module sram_1r1w #(
    parameter WORD_W = 64,
    parameter DEPTH  = 256
) (
    input  logic                     i_clk,
    input  logic [$clog2(DEPTH)-1:0] i_req_addr,
    input  logic                     i_req_wen,
    input  logic [WORD_W-1:0]        i_req_wdata,
    input  logic [WORD_W/8-1:0]      i_req_wstrb,
    output logic [WORD_W-1:0]        o_req_rdata
);
    logic [WORD_W-1:0] mem [0:DEPTH-1];

    always_ff @(posedge i_clk) begin
        if (i_req_wen) begin
            for (int i = 0; i < WORD_W/8; i++) begin
                if (i_req_wstrb[i]) begin
                    mem[i_req_addr][i*8 +: 8] <= i_req_wdata[i*8 +: 8];
                end
            end
        end else begin
            o_req_rdata <= mem[i_req_addr];
        end
    end
endmodule: sram_1r1w
