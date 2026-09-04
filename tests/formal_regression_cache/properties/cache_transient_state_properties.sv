`ifdef SYMBIYOSYS_COMPATIBLE
logic symbiyosys_past_valid;
initial begin
    symbiyosys_past_valid = 1'b0;
end
always_ff @(posedge i_clk) begin
    if (!symbiyosys_past_valid)
        assume (!i_rst_n);
    symbiyosys_past_valid <= 1'b1;
end
always_ff @(posedge i_clk) begin
    if (i_rst_n && symbiyosys_past_valid) begin
        transient_state_stable_until_completion_assert:
            assert (!$past(i_rst_n) ||
                    !$past(f_stage_valid && !f_stage_fire) ||
                    (f_stage_valid &&
                     (f_stage_addr == $past(f_stage_addr)) &&
                     (f_stage_requester == $past(f_stage_requester)) &&
                     (f_stage_trans_id == $past(f_stage_trans_id))));
    end
end
`else
property transient_state_stable_until_completion;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!$past(i_rst_n) ||
            !$past(f_stage_valid && !f_stage_fire) ||
            (f_stage_valid &&
             (f_stage_addr == $past(f_stage_addr)) &&
             (f_stage_requester == $past(f_stage_requester)) &&
             (f_stage_trans_id == $past(f_stage_trans_id))));
endproperty

transient_state_stable_until_completion_assert:
    assert property (transient_state_stable_until_completion);
`endif
