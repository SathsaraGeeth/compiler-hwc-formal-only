read_file -f files_stability.f
set_top directory_stability
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 8
prove stalled_response_stable
prove stalled_snoop_stable
