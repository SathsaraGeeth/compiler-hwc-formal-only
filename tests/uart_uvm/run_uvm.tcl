read_file -f files.f
set_top uart_tb_top
set_timescale 1ns/1ps
set_uvm_test uart_test
set_seed 100
simulate
