read_file -f files.f
set_top btor2_pipeline
create_clock clk
set_reset rst_n -active_low
set_engine bmc
set_formal_backend btormc
set_max_depth 8
set_formal_work_directory work/failing
set_formal_timeout 2m
set_expected_result counterexample

prove failing_grant_never
