read_file -f ../files_scoreboard_anvil_run.f
set_top cache_system_scoreboard_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 16
prove unique_outstanding_line_anvil
