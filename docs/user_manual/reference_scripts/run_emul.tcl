# read synthesizable RTL
read_file -f filelist.f

# hardware top
set_top fifo_top

# select the physical FPGA target (server comes from HWC_FPGA_SERVER)
set_target fpga

# run
emulate
