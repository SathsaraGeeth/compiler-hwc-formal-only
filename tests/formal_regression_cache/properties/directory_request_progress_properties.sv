logic [CACHE_ID_W-1:0] accepted_dst_id;
logic [TRANS_ID_W-1:0] accepted_trans_id;

always_ff @(posedge i_clk) begin
    if (!i_rst_n) begin
        accepted_dst_id  <= '0;
        accepted_trans_id <= '0;
    end else if (f_accepted_get) begin
        accepted_dst_id  <= f_req_src_id;
        accepted_trans_id <= f_req_trans_id;
    end
end

property accepted_request_eventually_gets_matching_response;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (f_accepted_get implies s_eventually
        (f_rsp_valid && f_rsp_ready &&
         (f_rsp_dst_id == accepted_dst_id) &&
         (f_rsp_trans_id == accepted_trans_id)));
endproperty

accepted_request_eventually_gets_matching_response_assert:
    assert property (accepted_request_eventually_gets_matching_response);
