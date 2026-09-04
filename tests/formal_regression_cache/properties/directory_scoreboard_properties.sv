`ifdef SYMBIYOSYS_COMPATIBLE
always_ff @(posedge i_clk)
    if (i_rst_n)
        unique_outstanding_line_reservation_assert:
            assert (!(f_reserve_0 && f_reserve_1));
`else
property unique_outstanding_line_reservation;
    @(posedge i_clk) disable iff (!i_rst_n)
    always !(f_reserve_0 && f_reserve_1);
endproperty

unique_outstanding_line_reservation_assert:
    assert property (unique_outstanding_line_reservation);
`endif
