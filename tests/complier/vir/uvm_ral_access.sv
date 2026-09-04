package uvm_ral_access_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class status_register extends uvm_reg;
        `uvm_object_utils(status_register)

        function new(string name = "status_register");
            super.new(name, 4, UVM_NO_COVERAGE);
        endfunction
    endclass

    class access_block extends uvm_reg_block;
        `uvm_object_utils(access_block)

        function new(string name = "access_block");
            super.new(name, UVM_NO_COVERAGE);
        endfunction
    endclass

    class access_test extends uvm_test;
        access_block block_model;
        status_register status_reg;
        uvm_reg_field clear_field;
        uvm_reg_field fixed_field;
        uvm_reg_field sticky_field;
        uvm_reg_field event_field;
        uvm_status_e status;
        uvm_reg_data_t value;
        `uvm_component_utils(access_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            block_model = access_block::type_id::create("block_model");
            status_reg = status_register::type_id::create("status_reg");
            clear_field = uvm_reg_field::type_id::create("clear_field");
            fixed_field = uvm_reg_field::type_id::create("fixed_field");
            sticky_field = uvm_reg_field::type_id::create("sticky_field");
            event_field = uvm_reg_field::type_id::create("event_field");
            block_model.configure(null, "");
            status_reg.configure(block_model, null, "");
            clear_field.configure(status_reg, 1, 0, "W1C", 0, 1, 1, 0, 0);
            fixed_field.configure(status_reg, 1, 1, "RO", 0, 1, 1, 0, 0);
            sticky_field.configure(status_reg, 1, 2, "W1S", 0, 0, 1, 0, 0);
            event_field.configure(status_reg, 1, 3, "RC", 0, 1, 1, 0, 0);
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            status_reg.write(status, 4'b0101);
            status_reg.read(status, value);
            assert (value == 4'b1110);
            status_reg.read(status, value);
            assert (value == 4'b0110);
            uvm_report_info("RAL_ACCESS", "policies completed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_ral_access_tb;
    import uvm_pkg::*;
    import uvm_ral_access_pkg::*;

    initial run_test("access_test");
endmodule
