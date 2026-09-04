read_file -f files.f
set_top channel_case_i_formal
create_clock i_clk
set_reset i_rst_n -active_low
set_engine pdr
set_formal_backend pono
set_formal_work_directory work/formal
set_formal_timeout 2m
prove output_has_one_matching_input
