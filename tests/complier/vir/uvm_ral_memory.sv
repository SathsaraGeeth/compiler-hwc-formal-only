package uvm_ral_memory_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    class byte_memory extends uvm_mem;
        `uvm_object_utils(byte_memory)

        function new(string name = "byte_memory");
            super.new(name, 16, 8, "RW", UVM_NO_COVERAGE);
        endfunction
    endclass

    class memory_block extends uvm_reg_block;
        `uvm_object_utils(memory_block)

        function new(string name = "memory_block");
            super.new(name, UVM_NO_COVERAGE);
        endfunction
    endclass

    class memory_test extends uvm_test;
        memory_block block_model;
        byte_memory memory_model;
        uvm_reg_map bus_map;
        uvm_status_e status;
        uvm_reg_data_t value;
        `uvm_component_utils(memory_test)

        function new(string name, uvm_component parent);
            super.new(name, parent);
        endfunction

        function void build_phase(uvm_phase phase);
            super.build_phase(phase);
            block_model = memory_block::type_id::create("block_model");
            memory_model = byte_memory::type_id::create("memory_model");
            block_model.configure(null, "");
            memory_model.configure(block_model, "");
            bus_map = block_model.create_map(
                "bus_map", 'h1000, 4, UVM_LITTLE_ENDIAN);
            bus_map.add_mem(memory_model, 'h100, "RW");
        endfunction

        task run_phase(uvm_phase phase);
            phase.raise_objection(this);
            memory_model.write(status, 3, 8'h5a);
            memory_model.read(status, 3, value);
            assert (status == UVM_IS_OK);
            assert (value == 8'h5a);
            uvm_report_info("RAL_MEMORY", "memory completed", UVM_LOW);
            phase.drop_objection(this);
        endtask
    endclass
endpackage

module uvm_ral_memory_tb;
    import uvm_pkg::*;
    import uvm_ral_memory_pkg::*;

    initial run_test("memory_test");
endmodule
