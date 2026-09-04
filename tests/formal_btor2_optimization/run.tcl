read_file -f files.f
set_top formal_btor2_optimization
create_clock clk
set_reset rst_n -active_low
set_engine auto
set_formal_work_directory work/formal
set_formal_timeout 2m
set_expected_result proved

prove optimized_protocol
