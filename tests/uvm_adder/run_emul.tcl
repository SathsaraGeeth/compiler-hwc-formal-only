read_file -f files.f
set_top adder_uvm_tb
set_uvm_test adder_test
set_seed 100
set_target fpga
emulate
