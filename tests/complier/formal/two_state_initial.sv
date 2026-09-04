module formal_initial_dut (
    input  logic clk,
    input  logic rst_n,
    output bit   initialized
);
    initial initialized = 1'b1;

    // Keep this as a hardware module while preserving the initial value.
    always_ff @(posedge clk)
        initialized <= initialized;
endmodule

module formal_initial_properties;
    logic clk;
    logic rst_n;
    bit initialized;

    formal_initial_dut dut (.*);

    property initial_block_runs;
        @(posedge clk) disable iff (!rst_n)
        initialized;
    endproperty

    assert property (initial_block_runs);
endmodule
