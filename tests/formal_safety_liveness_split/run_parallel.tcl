read_file -f files.f
set_top safety_liveness_split
set_engine auto
set_max_depth 20
prove trivial_safety_and_liveness
