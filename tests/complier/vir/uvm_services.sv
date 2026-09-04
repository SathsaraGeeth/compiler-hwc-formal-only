package uvm_services_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class request extends uvm_sequence_item;
        rand bit [7:0] data;
        `uvm_object_utils_begin(request)
            `uvm_field_int(data, UVM_DEFAULT)
        `uvm_object_utils_end

        function new(string name = "request");
            super.new(name);
        endfunction
    endclass

    class service_callback extends uvm_callback;
        `uvm_object_utils(service_callback)

        function new(string name = "service_callback");
            super.new(name);
        endfunction

        virtual function void applied();
            uvm_report_info("CALLBACK", "applied", UVM_LOW);
        endfunction
    endclass

    class agent extends uvm_component;
        request item;
        int timeout;
        `uvm_component_utils(agent)
        `uvm_register_cb(agent, service_callback)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            if (!uvm_config_db#(int)::get(this, "", "timeout", timeout))
                uvm_report_fatal("CONFIG", "timeout is missing");
            item = request::type_id::create("item");
            assert (timeout == 100);
        endfunction

        task run_phase(uvm_phase phase);
            `uvm_do_callbacks(agent, service_callback, applied())
        endtask
    endclass

    class services_test extends uvm_test;
        agent environment;
        service_callback callback;
        int resource_limit;
        `uvm_component_utils(services_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            uvm_config_db#(int)::set(this, "environment", "timeout", 100);
            uvm_resource_db#(int)::set("global", "limit", 55, this);
            if (!uvm_resource_db#(int)::read_by_name(
                    "global", "limit", resource_limit, this))
                uvm_report_fatal("RESOURCE", "limit is missing");
            assert (resource_limit == 55);
            environment = agent::type_id::create("environment", this);
            callback = service_callback::type_id::create("callback");
            uvm_callbacks#(agent, service_callback)::add(environment, callback);
        endfunction
    endclass
endpackage

module uvm_services_tb;
    import uvm_pkg::*;
    import uvm_services_pkg::*;

    initial run_test("services_test");
endmodule
