module sva_shapes(input logic clk, rst_n, req, ack, busy);
    default clocking cb @(posedge clk); endclocking

    property p_delay;
        disable iff (!rst_n) req |-> ##[1:3] ack;
    endproperty

    property p_repeat;
        first_match(req ##1 busy[*2:4]) |=> ack;
    endproperty

    property p_until;
        req until_with ack;
    endproperty

    property p_abort;
        accept_on (!rst_n) always (req |-> s_eventually [1:4] ack);
    endproperty

    property p_case;
        case ({req, busy})
            2'b10: ##1 ack;
            2'b11: ack throughout busy[*1:2];
            default: !ack;
        endcase
    endproperty

    property p_local;
        int matched;
        (req, matched = 1) ##1 (ack && matched == 1);
    endproperty

    property p_unbounded_delay;
        req |-> ##[1:$] ack;
    endproperty

    property p_unbounded_repeat;
        req |-> busy[*1:$] ##1 ack;
    endproperty

    property p_nonconsecutive;
        req |-> busy[=2] ##1 ack;
    endproperty

    property p_goto;
        req |-> busy[->2] ##1 ack;
    endproperty

    property p_zero_repeat;
        req |-> busy[*0:2] ##1 ack;
    endproperty

    property p_large_delay;
        req |-> ##[1:300] ack;
    endproperty

    p_delay_assert: assert property (p_delay);
    p_repeat_assume: assume property (p_repeat);
    p_until_cover: cover property (p_until);
    p_abort_restrict: restrict property (p_abort);
    p_case_assert: assert property (p_case);
    p_local_assert: assert property (p_local);
    p_unbounded_delay_assert: assert property (p_unbounded_delay);
    p_unbounded_repeat_assert: assert property (p_unbounded_repeat);
    p_nonconsecutive_assert: assert property (p_nonconsecutive);
    p_goto_assert: assert property (p_goto);
    p_zero_repeat_assert: assert property (p_zero_repeat);
    p_large_delay_assert: assert property (p_large_delay);
endmodule
