package frontend_feature_pkg;
    typedef enum logic [1:0] {idle, active, done} state_t;
    typedef struct packed {
        logic [7:0] payload;
        state_t     state;
    } packet_t;

    class transaction #(int WIDTH = 8);
        rand bit [WIDTH-1:0] data;
        rand int unsigned    delay;
        bit [WIDTH-1:0]      queue[$];
        bit [WIDTH-1:0]      dynamic_array[];

        constraint short_delay {
            delay inside {[0:15]};
        }

        covergroup values;
            coverpoint data;
            coverpoint delay;
        endgroup

        function new;
            values = new;
        endfunction

        virtual function void sample;
            values.sample();
        endfunction
    endclass
endpackage

interface frontend_bus #(int WIDTH = 8) (input logic clk);
    logic                 valid;
    logic                 ready;
    logic [WIDTH-1:0]     data;

    clocking driver_cb @(posedge clk);
        default input #1step output #0;
        output valid, data;
        input ready;
    endclocking

    modport device (input clk, valid, data, output ready);
    modport driver (clocking driver_cb);
endinterface

module frontend_device #(int WIDTH = 8) (frontend_bus.device bus);
    always_ff @(posedge bus.clk)
        bus.ready <= bus.valid;
endmodule

module frontend_full;
    import frontend_feature_pkg::*;

    logic clk = 1'b0;
    frontend_bus #(8) bus(clk);
    frontend_device #(8) dut(bus);
    transaction #(8) item;
    packet_t packets [2];

    for (genvar index = 0; index < 2; ++index) begin : generated
        logic [index:0] value;
    end

    property valid_eventually_ready;
        @(posedge clk) bus.valid |=> bus.ready;
    endproperty

    ready_check: assert property (valid_eventually_ready);

    initial begin
        item = new;
        void'(item.randomize());
        item.sample();
    end

    final begin
        $display("frontend complete");
    end
endmodule
