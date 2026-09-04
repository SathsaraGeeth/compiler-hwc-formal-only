read_file -f files_type_fair.f
set_top directory_type_fair
create_clock i_clk
set_reset i_rst_n -active_low
set_engine pdr
set_max_depth 80
prove uncacheable_request_justice
