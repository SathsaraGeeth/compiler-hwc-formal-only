read_file -f files.f
set_top btor2_pipeline
create_clock clk
set_reset rst_n -active_low
set_max_depth 4
set_formal_work_directory work/matrix
set_expected_result any

formal_task pono_bmc bmc pono
formal_task btormc_bmc bmc btormc
formal_matrix prove request_protocol
