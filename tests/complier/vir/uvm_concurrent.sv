package uvm_concurrent_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class slow_worker extends uvm_component;
        int payload;
        `uvm_component_utils(slow_worker)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            if (!uvm_config_db#(int)::get(this, "", "payload", payload))
                uvm_report_fatal("CONFIG", "slow payload is missing");
            assert (payload == 11);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            assert (payload == 11);
            uvm_report_info("SLOW", "start", UVM_LOW);
            #2;
            uvm_report_info("SLOW", "done", UVM_LOW);
            phase.drop_objection(this);
        endtask

        task main_phase(uvm_phase phase);
            phase.raise_objection(this);
            uvm_report_info("MAIN", "executed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass

    class fast_worker extends uvm_component;
        int payload;
        `uvm_component_utils(fast_worker)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            if (!uvm_config_db#(int)::get(this, "", "payload", payload))
                uvm_report_fatal("CONFIG", "fast payload is missing");
            assert (payload == 22);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            assert (payload == 22);
            uvm_report_info("FAST", "start", UVM_LOW);
            #1;
            uvm_report_info("FAST", "done", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass

    class concurrent_test extends uvm_test;
        slow_worker slow;
        fast_worker fast;
        `uvm_component_utils(concurrent_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            uvm_config_db#(int)::set(this, "slow", "payload", 11);
            uvm_config_db#(int)::set(this, "fast", "payload", 22);
            slow = slow_worker::type_id::create("slow", this);
            fast = fast_worker::type_id::create("fast", this);
        endfunction
    endclass
endpackage

module uvm_concurrent_tb;
    import uvm_pkg::*;
    import uvm_concurrent_pkg::*;

    initial run_test("concurrent_test");
endmodule
