`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        completed_read_observes_latest_write_assert:
            assert (!(f_read_complete && f_latest_write_valid) ||
                    (f_read_data == f_latest_write));
`else
property completed_read_observes_latest_write;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!(f_read_complete && f_latest_write_valid) ||
            (f_read_data == f_latest_write));
endproperty

completed_read_observes_latest_write_assert:
    assert property (completed_read_observes_latest_write);
`endif
