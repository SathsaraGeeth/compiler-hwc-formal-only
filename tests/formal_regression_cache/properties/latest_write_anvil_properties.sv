`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk) begin
    // Ignore unconstrained power-up outputs.  The observation starts only
    // after the deterministic client-0 store has entered the model.
    if (f_reset_n && r_store_accepted) begin
        client0_read_latest_anvil_assert:
            assert (!(f_rsp0_valid && f_rsp0_read) ||
                    (f_rsp0_data == f_rsp0_expected));
        client1_read_latest_anvil_assert:
            assert (!(f_rsp1_valid && f_rsp1_read) ||
                    (f_rsp1_data == f_rsp1_expected));
    end
end
`else
property client0_read_latest_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!(f_rsp0_valid && f_rsp0_read) ||
            (f_rsp0_data == f_rsp0_expected));
endproperty

property client1_read_latest_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!(f_rsp1_valid && f_rsp1_read) ||
            (f_rsp1_data == f_rsp1_expected));
endproperty

client0_read_latest_anvil_assert: assert property (client0_read_latest_anvil);
client1_read_latest_anvil_assert: assert property (client1_read_latest_anvil);

property no_bad_read_seen_anvil;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !f_bad_read_seen;
endproperty

no_bad_read_seen_anvil_assert: assert property (no_bad_read_seen_anvil);
`endif
