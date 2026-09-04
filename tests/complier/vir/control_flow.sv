module vir_control_flow;
    int value;
    event done;

    initial begin
        if (value == 0)
            value = 1;
        else
            value = 2;

        repeat (2)
            value = value + 1;

        while (value < 4)
            value = value + 1;

        for (int index = 0; index < 2; index++)
            value = value + index;

        case (value)
            4: value = 5;
            default: value = 6;
        endcase

        fork
            #1 -> done;
            wait (value == 6);
        join
    end
endmodule
