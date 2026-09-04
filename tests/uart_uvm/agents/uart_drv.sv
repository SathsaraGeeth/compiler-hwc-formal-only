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

class uart_drv extends uvm_driver #(uart_txn);
    `uvm_component_utils(uart_drv);

    virtual uart_if vif;

    function new(string name = "uart_drv", uvm_component parent = null);
        super.new(name, parent);
    endfunction

    function void build_phase(uvm_phase phase);
        super.build_phase(phase);
        if (!uvm_config_db#(virtual uart_if)::get(this, "", "vif", vif))
            `uvm_fatal("UART_DRV", "Virtual interface not set");
    endfunction

    task run_phase(uvm_phase phase);
        repeat (100) @(posedge vif.clk);
        fork
            forever begin
                handle_tx();
                handle_rx();
            end
        join_none
    endtask

    task handle_tx();
        uart_txn txn;
        uart_rst_txn rst_t;
        uart_tx_txn  tx_t;
        
        forever begin
            seq_item_port.get_next_item(txn);

            if ($cast(rst_t, txn)) begin
                vif.rst_n <= 0;
                repeat (2) @(posedge vif.clk);
                vif.rst_n <= 1;
                repeat (100) @(posedge vif.clk);
            end
            else if ($cast(tx_t, txn)) begin
                // wait for DUT ready
                do @(posedge vif.clk); while (!vif.enq_tx_ready);
                vif.enq_tx_valid <= 1;
                vif.enq_tx_data  <= tx_t.data;
                @(posedge vif.clk);
                vif.enq_tx_valid <= 0;
                vif.enq_tx_data  <= 0;
            end

            seq_item_port.item_done();
            @(posedge vif.clk);
            return;
        end
    endtask

    task handle_rx();
        forever begin
            @(posedge vif.clk);
            if (vif.deq_rx_ready) begin
                vif.deq_rx_valid <= 1;
                @(posedge vif.clk);
                vif.deq_rx_valid <= 0;
            end
            return ;
        end
    endtask

endclass: uart_drv