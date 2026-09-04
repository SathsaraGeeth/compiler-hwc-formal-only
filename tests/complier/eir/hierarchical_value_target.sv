module hierarchical_value_target (
    input  logic clk,
    input  logic select,
    output logic selected
);
    for (genvar i = 0; i < 2; i++) begin : EVENTS
        logic event_current;
    end

    assign EVENTS[0].event_current = select;
    assign EVENTS[1].event_current = !select;
    assign selected = EVENTS[0].event_current;

    property hierarchical_assignment_preserved;
        @(posedge clk) always selected == select;
    endproperty

    hierarchical_assignment_preserved_assert:
        assert property (hierarchical_assignment_preserved);
endmodule
