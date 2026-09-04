package uvm_tlm_test_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class packet extends uvm_sequence_item;
        `uvm_object_utils(packet)

        function new(string name = "packet");
            super.new(name);
        endfunction
    endclass

    class tlm_test extends uvm_test;
        uvm_tlm_fifo#(packet) fifo;
        packet sent;
        packet received;
        `uvm_component_utils(tlm_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            sent = packet::type_id::create("sent");
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            fork
                begin
                    #1;
                    fifo.put(sent);
                end
                begin
                    fifo.get(received);
                    uvm_report_info("TLM", "received", UVM_LOW);
                end
            join
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_tlm_tb;
    import uvm_pkg::*;
    import uvm_tlm_test_pkg::*;

    initial run_test("tlm_test");
endmodule
