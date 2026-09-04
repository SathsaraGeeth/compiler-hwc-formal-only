package uvm_reporting_test_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class reporting_test extends uvm_test;
        `uvm_component_utils(reporting_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            set_report_verbosity_level(UVM_LOW);
            uvm_report_info("FILTERED", "must not be printed", UVM_HIGH);
            set_report_id_verbosity("VISIBLE", UVM_HIGH);
            uvm_report_info("VISIBLE", "id override", UVM_HIGH);
            set_report_id_action("IGNORED", UVM_NO_ACTION);
            uvm_report_error("IGNORED", "must not fail");
            set_report_severity_id_action(
                UVM_WARNING, "VISIBLE_WARNING", UVM_DISPLAY);
            uvm_report_warning("VISIBLE_WARNING", "action override");
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_reporting_tb;
    import uvm_pkg::*;
    import uvm_reporting_test_pkg::*;

    initial run_test("reporting_test");
endmodule
