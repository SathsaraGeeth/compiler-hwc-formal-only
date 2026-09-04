read_file -f ../files_fairness_anvil_run.f
set_top fairness_anvil
create_clock i_clk
set_reset i_rst_n -active_low
set_engine pdr
set_max_depth 80
prove client0_service_fairness_anvil
prove client1_service_fairness_anvil
