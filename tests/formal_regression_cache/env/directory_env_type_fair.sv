property downstream_progress;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (s_eventually
        (f_cache_rsp_ready && f_cache_snp_ready &&
         f_lock_rsp_ready && f_mem_req_ready && f_mem_rsp_valid));
endproperty

downstream_progress_assume:
    assume property (downstream_progress);
