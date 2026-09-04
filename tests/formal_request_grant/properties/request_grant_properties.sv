module formal_request_grant_properties;
    logic clk;
    logic rst_n;
    logic request;
    logic ready;
    logic busy;
    logic grant;

    request_grant_fsm dut (
        .clk,
        .rst_n,
        .request,
        .ready,
        .busy,
        .grant
    );

    property request_grant_contract;
        @(posedge clk) disable iff (!rst_n)
        (always (busy iff !ready)) and
        (always (grant implies (busy and !ready))) and
        (always ((request and ready) implies s_eventually grant));
    endproperty

    request_grant_contract_assert:
        assert property (request_grant_contract);
endmodule
