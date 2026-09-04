# include directories
+incdir+rtl/include
+incdir+tb/include
+incdir+formal/include

# rtl sources
rtl/pkg/types_pkg.sv
rtl/fifo.sv
rtl/memory.sv
rtl/top.sv

# testbench sources
tb/interfaces/fifo_if.sv
tb/tb_top.sv
tb/tests/base_test.sv

# formal properties
formal/fifo_assertions.sv
