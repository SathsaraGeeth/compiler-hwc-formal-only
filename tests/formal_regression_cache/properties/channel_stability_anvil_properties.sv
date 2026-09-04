property stalled_channel_stable_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!f_was_stalled ||
            (f_rsp_valid && (f_rsp_data == f_stalled_data)));
endproperty

stalled_channel_stable_anvil_assert:
    assert property (stalled_channel_stable_anvil);
