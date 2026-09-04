module event_display_ram (
    input  logic       clk,
    input  logic [1:0] address,
    output logic [7:0] data
);
    logic [7:0] memory [0:3];

    initial begin
        for (int i = 0; i < 4; i++) begin
            memory[i] = 8'h10 + i;
        end
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule

module vir_event_display;
    logic       clk;
    logic [1:0] address;
    logic [7:0] data;

    event_display_ram dut (.*);

    always #1 clk = ~clk;

    initial begin
        clk = 0;
        address = 2;
        @(posedge clk);
        @(posedge clk);
        $display("data=%h time=%0t", data, $time);
        #1;
        $finish;
    end
endmodule
