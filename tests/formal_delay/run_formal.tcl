read_file -f files.f
set_top delayed_ack_properties
set_engine bmc
set_max_depth 8
prove delayed_ack_correct
