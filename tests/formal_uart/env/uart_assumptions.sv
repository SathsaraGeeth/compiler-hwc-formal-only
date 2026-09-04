property reset_before_past_valid;
    @(posedge clk)
    !f_past_valid |-> !rst_n;
endproperty

assume property (reset_before_past_valid);
