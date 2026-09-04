read_file -f files.f
set_top circular_buffer_uvm_tb
set_timescale 1ns/1ps
set_uvm_test circular_buffer_test
set_seed 100
simulate
