`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk) begin
    if (i_rst_n) begin
        stalled_response_stable_assert:
            assert (!(f_rsp_valid && !f_rsp_ready) ||
                    (f_rsp_next_valid && (f_rsp_next == f_rsp)));
        stalled_snoop_stable_assert:
            assert (!(f_snp_valid && !f_snp_ready) ||
                    (f_snp_next_valid && (f_snp_next == f_snp)));
    end
end
`else
property stalled_response_stable;
    @(posedge i_clk) disable iff (!i_rst_n)
    always ((f_rsp_valid && !f_rsp_ready) ->
            (f_rsp_next_valid && (f_rsp_next == f_rsp)));
endproperty

property stalled_snoop_stable;
    @(posedge i_clk) disable iff (!i_rst_n)
    always ((f_snp_valid && !f_snp_ready) ->
            (f_snp_next_valid && (f_snp_next == f_snp)));
endproperty

assert property (stalled_response_stable);
assert property (stalled_snoop_stable);
`endif
