read_file -f ../files_stability_anvil_run.f
set_top channel_stability_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 12
prove stalled_channel_stable_anvil
