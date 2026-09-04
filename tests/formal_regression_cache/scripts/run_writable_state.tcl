read_file -f files_writable_state.f
set_top cache_writable_state
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 24
prove write_requires_writable_state
