package uart_uvm_pkg;
    import uvm_pkg::*;
    `include "uvm_macros.svh"

    `include "uart_txn.sv"
    `include "uart_seq.sv"
    `include "uart_seqr.sv"
    `include "uart_drv.sv"
    `include "uart_mon.sv"
    `include "uart_scb.sv"
    `include "uart_cov.sv"
    `include "uart_agent.sv"
    `include "uart_env.sv"
    `include "uart_test.sv"
endpackage
