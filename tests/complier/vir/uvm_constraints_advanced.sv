package uvm_constraints_advanced_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class advanced_item extends uvm_sequence_item;
        rand bit [3:0] kind;
        rand bit [7:0] value;
        rand bit [7:0] mirror;

        constraint advanced {
            kind dist {0 := 1, [1:3] :/ 6};
            (kind == 0) -> value inside {[1:4]};
            if (kind != 0)
                value inside {[8:15]};
            solve kind before value;
            unique {value, mirror};
        }
        `uvm_object_utils(advanced_item)

        function new(string name = "advanced_item");
            super.new(name);
        endfunction
    endclass

    class advanced_constraint_test extends uvm_test;
        advanced_item item;
        `uvm_component_utils(advanced_constraint_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            item = advanced_item::type_id::create("item");
            repeat (16)
                assert (item.randomize());
            uvm_report_info("ADVANCED_CONSTRAINT", "completed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_constraints_advanced_tb;
    import uvm_pkg::*;
    import uvm_constraints_advanced_pkg::*;

    initial run_test("advanced_constraint_test");
endmodule
