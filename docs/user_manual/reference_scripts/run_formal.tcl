# read RTL + properties
read_file -f filelist.f

# formal top
set_top fifo_top

# clock definition
create_clock clk

# reset definition
set_reset rst_n -active_low

# engine: auto, bmc, kind, pdr, smt
set_engine bmc

# backend: auto, pono, btormc, ric3, avr, abc, avy, suprove, smtbmc
set_formal_backend btormc

set_max_depth 50
set_formal_work_directory work/formal
set_formal_timeout 2m
set_expected_result any
set_cover_trace shortest

# task matrix
# formal_task bounded bmc btormc
# formal_task unbounded pdr ric3
# formal_matrix prove property_name

# prove assertions
prove -all
# prove property_name

# cover properties
cover -all
# cover property_name
