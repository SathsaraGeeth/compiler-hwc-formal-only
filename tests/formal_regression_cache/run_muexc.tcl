read_file -f files_muexc.f
set_top directory_muexc
create_clock i_clk
set_reset i_rst_n -active_low
set_engine pdr
set_formal_work_directory work/muexc
set_formal_timeout 2m
prove muexc_cache_req_accepted
