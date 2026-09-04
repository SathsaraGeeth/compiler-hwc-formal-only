`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk) begin
    if (i_rst_n) begin
        direct_grant_cover: cover (f_direct_grant_reached);
        snoop_ack_grant_cover: cover (f_snoop_ack_grant_reached);
        both_clients_accepted_cover: cover (f_both_clients_accepted);
        bus_lock_response_cover: cover (f_bus_lock_response_reached);
    end
end
`else
property reach_direct_grant_path;
    @(posedge i_clk) disable iff (!i_rst_n)
    f_direct_grant_reached;
endproperty

property reach_snoop_ack_grant_path;
    @(posedge i_clk) disable iff (!i_rst_n)
    f_snoop_ack_grant_reached;
endproperty

property reach_both_clients_accepted;
    @(posedge i_clk) disable iff (!i_rst_n)
    f_both_clients_accepted;
endproperty

property reach_bus_lock_response;
    @(posedge i_clk) disable iff (!i_rst_n)
    f_bus_lock_response_reached;
endproperty

direct_grant_cover:
    cover property (reach_direct_grant_path);
snoop_ack_grant_cover:
    cover property (reach_snoop_ack_grant_path);
both_clients_accepted_cover:
    cover property (reach_both_clients_accepted);
bus_lock_response_cover:
    cover property (reach_bus_lock_response);
`endif
