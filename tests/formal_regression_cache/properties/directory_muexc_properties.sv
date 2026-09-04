`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk) begin
    if (i_rst_n) begin
        muexc_cache_req_accepted_assert:
            assert (!((f_req_valid_0 && f_req_ready_0) &&
                      (f_req_valid_1 && f_req_ready_1)));
    end
end
`else
property muexc_cache_req_accepted;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !((f_req_valid_0 && f_req_ready_0) &&
             (f_req_valid_1 && f_req_ready_1));
endproperty

muexc_cache_req_accepted_assert:
    assert property (muexc_cache_req_accepted);
`endif
