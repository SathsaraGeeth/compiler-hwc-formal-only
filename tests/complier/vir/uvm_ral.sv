package uvm_ral_test_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class control_register extends uvm_reg;
        `uvm_object_utils(control_register)

        function new(string name = "control_register");
            super.new(name, 8, UVM_NO_COVERAGE);
        endfunction
    endclass

    class register_block extends uvm_reg_block;
        `uvm_object_utils(register_block)

        function new(string name = "register_block");
            super.new(name, UVM_NO_COVERAGE);
        endfunction
    endclass

    class ral_test extends uvm_test;
        register_block   block_model;
        control_register control;
        uvm_reg_field    enable;
        uvm_reg_map      bus_map;
        uvm_status_e     status;
        uvm_reg_data_t   value;
        `uvm_component_utils(ral_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            block_model = register_block::type_id::create("block_model");
            control = control_register::type_id::create("control");
            enable = uvm_reg_field::type_id::create("enable");
            block_model.configure(null, "");
            control.configure(block_model, null, "");
            enable.configure(control, 1, 0, "RW", 0, 1, 1, 1, 0);
            bus_map = block_model.create_map(
                "bus_map", 0, 4, UVM_LITTLE_ENDIAN);
            bus_map.add_reg(control, 'h0, "RW");
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            control.write(status, 8'h5a);
            control.read(status, value);
            assert (status == UVM_IS_OK);
            assert (value == 8'h5a);
            assert (control.predict(8'ha5));
            control.read(status, value);
            assert (value == 8'ha5);
            block_model.reset();
            control.read(status, value);
            assert (value == 8'h01);
            control.set(8'h12);
            assert (control.needs_update());
            assert (control.get() == 8'h12);
            assert (control.get_mirrored_value() == 8'h01);
            control.update(status);
            assert (!control.needs_update());
            control.poke(status, 8'h34);
            control.peek(status, value);
            assert (value == 8'h34);
            uvm_report_info("RAL", "register model completed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_ral_tb;
    import uvm_pkg::*;
    import uvm_ral_test_pkg::*;

    initial run_test("ral_test");
endmodule
