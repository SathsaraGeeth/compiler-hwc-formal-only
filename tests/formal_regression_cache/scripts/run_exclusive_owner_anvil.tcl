read_file -f ../files_exclusive_owner_anvil_run.f
set_top directory_owner_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 8
prove exclusive_owner_has_no_sharers_anvil
