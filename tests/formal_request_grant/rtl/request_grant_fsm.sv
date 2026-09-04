module request_grant_fsm (
    input  logic clk,
    input  logic rst_n,
    input  logic request,
    output logic ready,
    output logic busy,
    output logic grant
);
    typedef enum logic [1:0] {
        idle,
        queued,
        granting
    } state_t;

    state_t state;

    always_ff @(posedge clk) begin
        if (!rst_n)
            state <= idle;
        else begin
            unique case (state)
            idle:     state <= request ? queued : idle;
            queued:   state <= granting;
            granting: state <= idle;
            default:  state <= idle;
            endcase
        end
    end

`ifdef INJECT_FAULT
    // controller advertises availability while
    // it is producing a grant for the previous transaction.
    assign ready = state == idle || state == granting;
`else
    assign ready = state == idle;
`endif
    assign busy  = state != idle;
    assign grant = state == granting;
endmodule
