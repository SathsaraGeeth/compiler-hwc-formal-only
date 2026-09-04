read_file -f files_scoreboard.f
set_top directory_scoreboard
create_clock i_clk
set_reset i_rst_n -active_low
set_engine bmc
set_max_depth 8
prove unique_outstanding_line_reservation

