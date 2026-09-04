# read design files
read_file -f filelist.f

# UVM testbench top
set_top tb_top

# select UVM test
set_uvm_test fifo_test

# optional seed
set_seed 100

# run
simulate
