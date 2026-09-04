logic       f_pending;
logic [7:0] f_expected_data;

always_ff @(posedge i_clk) begin
    if (!i_rst_n) begin
        f_pending       <= 1'b0;
        f_expected_data <= '0;
    end else begin
        case ({f_accept, (f_out_valid && f_rsp_ready)})
            2'b10: begin
                f_pending       <= 1'b1;
                f_expected_data <= f_input_data;
            end
            2'b01, 2'b11: f_pending <= 1'b0;
            default: f_pending <= f_pending;
        endcase
    end
end

property output_has_one_matching_input;
    @(posedge i_clk) disable iff (!i_rst_n)
    always ((f_out_valid && f_rsp_ready) ->
            ((f_pending && (f_out_data == f_expected_data)) ||
             (!f_pending && f_accept && (f_out_data == f_input_data))));
endproperty

property no_duplicate_outstanding_input;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (f_accept -> (!f_pending || (f_out_valid && f_rsp_ready)));
endproperty

property fifo_order_preserved;
    @(posedge i_clk) disable iff (!i_rst_n)
    always ((f_out_valid && f_rsp_ready) ->
            ((f_pending && (f_out_data == f_expected_data)) ||
             (!f_pending && f_accept && (f_out_data == f_input_data))));
endproperty

property output_stable_under_backpressure;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (!$past(i_rst_n && f_out_valid && !f_rsp_ready) ||
            (f_out_valid && (f_out_data == $past(f_out_data))));
endproperty

property occupancy_within_boundary_capacity;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !(f_pending && f_accept && !(f_out_valid && f_rsp_ready));
endproperty

property accepted_item_eventually_leaves;
    @(posedge i_clk) disable iff (!i_rst_n)
    always (f_accept implies s_eventually (f_out_valid && f_rsp_ready));
endproperty

property downstream_eventually_ready;
    @(posedge i_clk) disable iff (!i_rst_n)
    always s_eventually f_rsp_ready;
endproperty

assert property (output_has_one_matching_input);
assert property (no_duplicate_outstanding_input);
assert property (fifo_order_preserved);
assert property (output_stable_under_backpressure);
assert property (occupancy_within_boundary_capacity);
assume property (downstream_eventually_ready);
assert property (accepted_item_eventually_leaves);
