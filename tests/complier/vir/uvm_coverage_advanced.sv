package uvm_coverage_advanced_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class coverage_item extends uvm_sequence_item;
        rand bit [3:0] kind;
        rand bit [7:0] value;

        constraint legal {
            kind inside {[0:3]};
            value inside {[0:3], [8:15]};
        }

        covergroup values;
            kind: coverpoint kind {
                bins kinds[] = {[0:3]};
            }
            value: coverpoint value {
                bins low = {[0:3]};
                bins high = {[8:15]};
                ignore_bins ignored = {6};
                illegal_bins forbidden = {7};
            }
            kind_value: cross kind, value;
        endgroup
        `uvm_object_utils(coverage_item)

        function new(string name = "coverage_item");
            super.new(name);
            values = new;
        endfunction

        function void sample();
            values.sample();
        endfunction
    endclass

    class coverage_test extends uvm_test;
        coverage_item item;
        `uvm_component_utils(coverage_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            item = coverage_item::type_id::create("item");
            repeat (16) begin
                assert (item.randomize());
                item.sample();
            end
            uvm_report_info("COVERAGE", "bins completed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_coverage_advanced_tb;
    import uvm_pkg::*;
    import uvm_coverage_advanced_pkg::*;

    initial run_test("coverage_test");
endmodule
