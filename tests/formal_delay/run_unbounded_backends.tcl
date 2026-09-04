read_file -f files.f
set_top delayed_ack_properties
set_engine pdr
set_max_depth 8

set_formal_backend ric3
prove delayed_ack_correct

set_formal_backend avr
prove delayed_ack_correct

set_formal_backend auto
prove delayed_ack_correct
