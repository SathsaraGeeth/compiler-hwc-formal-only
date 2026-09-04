read_file -f ../files_write_state_anvil_run.f
set_top write_state_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 12
prove write_requires_writable_state_anvil
