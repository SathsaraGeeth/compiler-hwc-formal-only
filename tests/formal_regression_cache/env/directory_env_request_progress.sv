property response_sink_makes_progress;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (s_eventually f_rsp_ready);
endproperty

response_sink_makes_progress_assume:
    assume property (response_sink_makes_progress);
