read_file -f files.f
set_top uart_formal
create_clock i_clk
set_reset i_rst_n -active_low
write_artifacts generated
