package uvm_sequence_services_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class response_item extends uvm_sequence_item;
        `uvm_object_utils(response_item)

        function new(string name = "response_item");
            super.new(name);
        endfunction
    endclass

    class service_sequence extends uvm_sequence#(response_item);
        response_item response;
        response_item received;
        `uvm_object_utils(service_sequence)

        function new(string name = "service_sequence");
            super.new(name);
        endfunction

        task body();
            lock(null);
            unlock(null);
            grab(null);
            ungrab(null);
            wait_for_grant(-1, 1);
            unlock(null);
            response = response_item::type_id::create("response");
            put_response(response);
            get_response(received);
            assert (received != null);
            uvm_report_info("SEQUENCE_SERVICE", "completed", UVM_LOW);
        endtask
    endclass

    class sequence_service_test extends uvm_test;
        service_sequence seq;
        `uvm_component_utils(sequence_service_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            seq = service_sequence::type_id::create("sequence");
            seq.start(null);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_sequence_services_tb;
    import uvm_pkg::*;
    import uvm_sequence_services_pkg::*;

    initial run_test("sequence_service_test");
endmodule
