package uvm_constraints_test_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class constrained_item extends uvm_sequence_item;
        rand bit [7:0] low;
        rand bit [7:0] high;
        randc bit [2:0] tag;

        constraint legal_values {
            low inside {[1:3], [8:9]};
            high > low;
            high <= 12;
        }
        `uvm_object_utils(constrained_item)

        function new(string name = "constrained_item");
            super.new(name);
        endfunction
    endclass

    class constraint_test extends uvm_test;
        constrained_item item;
        `uvm_component_utils(constraint_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            item = constrained_item::type_id::create("item");
            repeat (16) begin
                assert (item.randomize());
            end
            uvm_report_info("CONSTRAINT", "relations completed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_constraints_tb;
    import uvm_pkg::*;
    import uvm_constraints_test_pkg::*;

    initial run_test("constraint_test");
endmodule
