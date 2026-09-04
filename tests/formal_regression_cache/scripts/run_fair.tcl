read_file -f files_fair.f
set_top directory_fair_coi
create_clock i_clk
set_reset i_rst_n -active_low
set_engine pdr
set_max_depth 80
prove cache_0_justice
prove cache_1_justice
prove cache_0_strong_fairness
prove cache_1_strong_fairness
