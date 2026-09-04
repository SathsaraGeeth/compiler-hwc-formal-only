read_file -f ../files_latest_write_anvil_run.f
set_top latest_write_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 40
prove client1_read_latest_anvil
