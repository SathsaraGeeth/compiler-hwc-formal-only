/* FG(uncacheable request) -> GF(uncacheable acceptance) */
property uncacheable_request_justice;
    @(posedge i_clk) disable iff (!i_rst_n)
    (s_eventually (always f_uncacheable_valid)) implies
        (always (s_eventually
            (f_uncacheable_valid && f_uncacheable_ready)));
endproperty

uncacheable_request_justice_assert:
    assert property (uncacheable_request_justice);
