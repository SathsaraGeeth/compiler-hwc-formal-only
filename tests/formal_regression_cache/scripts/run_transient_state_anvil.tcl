read_file -f ../files_transient_state_anvil_run.f
set_top transient_state_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 16
prove retained_transaction_stable_anvil
