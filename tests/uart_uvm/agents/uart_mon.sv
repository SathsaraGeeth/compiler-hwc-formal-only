/*
SPDX-License-Identifier: Apache-2.0

Copyright 2026 Geeth Sathsara

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

import uvm_pkg::*;
`include "uvm_macros.svh"

class uart_mon extends uvm_monitor;
    `uvm_component_utils(uart_mon)

    virtual uart_if vif;
    uvm_analysis_port #(uart_txn) before_ap;
    uvm_analysis_port #(uart_txn) after_ap;


    function new(string name = "uart_mon", uvm_component parent);
        super.new(name, parent);
        before_ap = new("before_ap", this);
        after_ap  = new("after_ap", this);
    endfunction

    function void build_phase(uvm_phase phase);
        super.build_phase(phase);

        if (!uvm_config_db#(virtual uart_if)::get(this, "", "vif", vif))
            `uvm_fatal("UART_MON", "Virtual interface not set");
    endfunction

    task run_phase(uvm_phase phase);
        fork
            monitor_rst();
            monitor_rx();
            monitor_tx();
        join_none
    endtask

    task monitor_rst();
        uart_rst_txn rst;
        bit reset_sent;
        forever begin
            @(posedge vif.clk);
            if (!vif.rst_n && !reset_sent) begin
                rst = uart_rst_txn::type_id::create("rst_txn");
                after_ap.write(rst);
                reset_sent = 1;
            end
            if (vif.rst_n)
                reset_sent = 0;
        end
    endtask

    task monitor_rx();
        uart_rx_txn rx_t;
        int unsigned seq_id = 0;
        forever begin
            @(posedge vif.clk);
            if (vif.deq_rx_valid && vif.deq_rx_ready) begin
                rx_t = uart_rx_txn::type_id::create("rx_txn");
                rx_t.data   = vif.deq_rx_data;
                rx_t.seq_number = seq_id++;
                after_ap.write(rx_t);
            end
        end
    endtask

    task monitor_tx();
        uart_tx_txn tx_t;
        int unsigned seq_id = 0;
        forever begin
            @(posedge vif.clk);
            if (vif.enq_tx_valid && vif.enq_tx_ready) begin
                tx_t = uart_tx_txn::type_id::create("tx_txn");
                tx_t.data = vif.enq_tx_data;
                tx_t.seq_number = seq_id++;
                before_ap.write(tx_t);
            end
        end
    endtask
endclass
