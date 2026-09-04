module vir_control_flow_linear;
    int value = 0;

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
    end
endmodule
