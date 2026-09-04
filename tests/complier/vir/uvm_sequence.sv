package uvm_sequence_test_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class packet extends uvm_sequence_item;
        rand bit [7:0] data;
        constraint data_range { data inside {[1:10]}; }
        covergroup values;
            coverpoint data;
        endgroup
        `uvm_object_utils(packet)

        function new(string name = "packet");
            super.new(name);
            values = new;
        endfunction

        function void sample();
            values.sample();
        endfunction
    endclass

    class packet_sequence extends uvm_sequence#(packet);
        packet request;
        `uvm_object_utils(packet_sequence)

        function new(string name = "packet_sequence");
            super.new(name);
        endfunction

        task body();
            request = packet::type_id::create("request");
            assert (request.randomize());
            request.sample();
            start_item(request);
            finish_item(request);
            uvm_report_info("SEQUENCE", "body completed", UVM_LOW);
        endtask
    endclass

    class sequence_test extends uvm_test;
        packet_sequence seq;
        `uvm_component_utils(sequence_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            seq = packet_sequence::type_id::create("sequence");
            seq.start(null);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_sequence_tb;
    import uvm_pkg::*;
    import uvm_sequence_test_pkg::*;

    initial run_test("sequence_test");
endmodule
