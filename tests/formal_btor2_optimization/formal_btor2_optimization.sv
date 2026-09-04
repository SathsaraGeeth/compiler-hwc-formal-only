module formal_btor2_optimization (
    input  logic       clk,
    input  logic       rst_n,
    input  logic       request,
    input  logic [7:0] data,
    output logic       grant
);
    logic [7:0] add_zero_a;
    logic [7:0] add_zero_b;
    logic [7:0] multiply_four;
    logic [7:0] unused_logic;
    logic       selected_request;

    assign add_zero_a     = data + 8'h00;       // algebraic simplification
    assign add_zero_b     = data + 8'h00;       // common subexpression
    assign multiply_four  = data * 8'h04;       // strength reduction to sll
    assign selected_request = request & (add_zero_a == add_zero_b) &
                              (multiply_four == multiply_four);
                                                   // CSE then algebraic simplification
    assign unused_logic   = (data & 8'hff) ^ 8'h00;
                                                   // dead code elimination

    always_ff @(posedge clk) begin
        if (!rst_n)
            grant <= 1'b0;
        else
            grant <= selected_request;
    end

    property optimized_protocol;
        @(posedge clk) disable iff (!rst_n)
        (always (request implies request)) and
        (always (request implies s_eventually grant)) and
        (always (request implies s_eventually grant));
    endproperty

    optimized_protocol_assert:
        assert property (optimized_protocol);
endmodule
