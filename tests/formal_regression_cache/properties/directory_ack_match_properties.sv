`ifdef SYMBIYOSYS_COMPATIBLE
logic symbiyosys_ack_past_valid;
initial begin
    symbiyosys_ack_past_valid = 1'b0;
end
always_ff @(posedge i_clk) begin
    if (!symbiyosys_ack_past_valid)
        assume (!i_rst_n);
    symbiyosys_ack_past_valid <= 1'b1;
end
always_ff @(posedge i_clk) begin
    if (i_rst_n) begin
        only_matching_snoop_ack_completes_assert:
            assert (!(f_snoop_complete && !f_ack_match));
        if (symbiyosys_ack_past_valid) begin
            conflicting_grant_follows_matching_snoop_ack_assert:
                assert (!(f_conflicting_grant && $past(f_wait_valid)) ||
                        $past(f_ack_valid && f_ack_match));
        end
    end
end
`else
property only_matching_snoop_ack_completes;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !(f_snoop_complete && !f_ack_match);
endproperty

assert property (only_matching_snoop_ack_completes);

property conflicting_grant_follows_matching_snoop_ack;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!(f_conflicting_grant && $past(f_wait_valid)) ||
            $past(f_ack_valid && f_ack_match));
endproperty

assert property (conflicting_grant_follows_matching_snoop_ack);
`endif
