read_file -f files_latest_write.f
set_top cache_latest_write
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 8
prove completed_read_observes_latest_write
