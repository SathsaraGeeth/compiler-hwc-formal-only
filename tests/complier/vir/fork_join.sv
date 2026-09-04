module vir_fork_join;
    int join_a;
    int join_b;
    int any_slow;
    int any_fast;
    int none_slow;
    int none_fast;

    initial begin
        fork
            #2 join_a = 1;
            #1 join_b = 1;
        join
        assert (join_a == 1);
        assert (join_b == 1);

        fork
            #2 any_slow = 1;
            #1 any_fast = 1;
        join_any
        assert (any_slow == 0);
        assert (any_fast == 1);

        fork
            #2 none_slow = 1;
            #1 none_fast = 1;
        join_none
        assert (none_slow == 0);
        assert (none_fast == 0);
    end
endmodule
