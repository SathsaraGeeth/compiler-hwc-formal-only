read_file -f ../files_muexc_anvil_run.f
set_top directory_muexc_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 8
prove muexc_cache_req_accepted_anvil
