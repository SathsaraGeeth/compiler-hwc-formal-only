read_file -f files.f
set_top btor2_pipeline
create_clock clk
set_reset rst_n -active_low
set_engine auto
set_formal_work_directory work/formal
set_formal_timeout 2m
set_expected_result proved
set_cover_trace shortest

prove request_protocol
