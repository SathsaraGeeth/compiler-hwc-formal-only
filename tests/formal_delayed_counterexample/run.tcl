read_file -f files.f
set_top delayed_handshake
create_clock clk
set_reset rst_n -active_low
set_engine bmc
set_formal_backend btormc
set_max_depth 20
set_formal_work_directory work/formal
set_formal_timeout 2m
set_expected_result counterexample

prove request_gets_timely_grant
