read_file -f ../files_ack_order_anvil_run.f
set_top directory_ack_order_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 12
prove matching_ack_precedes_grant_anvil
