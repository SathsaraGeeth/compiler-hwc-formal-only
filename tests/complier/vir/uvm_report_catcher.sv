package uvm_report_catcher_test_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class suppress_catcher extends uvm_report_catcher;
        `uvm_object_utils(suppress_catcher)

        function new(string name = "suppress_catcher");
            super.new(name);
        endfunction

        virtual function action_e catch();
            if (get_id() == "SUPPRESS")
                return CAUGHT;
            if (get_id() == "DOWNGRADE") begin
                set_severity(UVM_WARNING);
                set_id("CHANGED");
                set_message("changed text");
                return THROW;
            end
            return THROW;
        endfunction
    endclass

    class catcher_test extends uvm_test;
        suppress_catcher catcher;
        `uvm_component_utils(catcher_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            catcher = suppress_catcher::type_id::create("catcher");
            uvm_report_cb::add(null, catcher);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            uvm_report_error("SUPPRESS", "caught error");
            uvm_report_error("DOWNGRADE", "original text");
            uvm_report_info("VISIBLE", "uncaught report", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_report_catcher_tb;
    import uvm_pkg::*;
    import uvm_report_catcher_test_pkg::*;

    initial run_test("catcher_test");
endmodule
