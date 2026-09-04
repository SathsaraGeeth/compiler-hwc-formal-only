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

class uart_scb extends uvm_scoreboard;
    `uvm_component_utils(uart_scb)

    uvm_analysis_export #(uart_txn) before_export;
    uvm_tlm_analysis_fifo #(uart_txn) before_fifo;

    uvm_analysis_export #(uart_txn) after_export;
    uvm_tlm_analysis_fifo #(uart_txn) after_fifo;

    uart_tx_txn before_list[$];
    int unsigned matched_count;
    int unsigned mismatch_count;
    int unsigned reset_count;

    function new(string name="uart_scb", uvm_component parent=null);
        super.new(name,parent);
    endfunction

    function void build_phase(uvm_phase phase);
        super.build_phase(phase);
        before_export = new("before_export", this);
        after_export  = new("after_export",  this);
        before_fifo   = new("before_fifo", this);
        after_fifo    = new("after_fifo",  this);
    endfunction

    function void connect_phase(uvm_phase phase);
        before_export.connect(before_fifo.analysis_export);
        after_export.connect(after_fifo.analysis_export);
    endfunction

    task run_phase(uvm_phase phase);
        fork
            consume_before();
            compare();
        join_none
    endtask

    task consume_before();
        uart_txn txn;
        uart_tx_txn tx_t;

        forever begin
            before_fifo.get(txn);
            if ($cast(tx_t, txn)) begin
                before_list.push_back(tx_t);
            end
        end
    endtask

    // Direct compare
    // task compare();
    //     uart_txn txn;
    //     uart_rst_txn rst_t;
    //     uart_rx_txn  rx_t;
    //     uart_tx_txn  tx_t;
    //     bit matched;
    //     int i;

    //     forever begin
    //         after_fifo.get(txn);

    //         if ($cast(rst_t, txn)) begin
    //             `uvm_info("SCB_AFTER", "RESET observed", UVM_LOW)
    //         end
    //         else if ($cast(rx_t, txn)) begin
    //             matched = 0;

    //             for (i = 0; i < before_list.size(); i++) begin
    //                 tx_t = before_list[i];
    //                 if (tx_t.seq_number == rx_t.seq_number) begin
    //                     matched = 1;
    //                     if (tx_t.data == rx_t.data)
    //                         `uvm_info("UART_SCB", $sformatf("TX[%0d] matches RX[%0d]=0x%0h", tx_t.seq_number, rx_t.seq_number, rx_t.data), UVM_LOW)
    //                     else
    //                         `uvm_error("UART_SCB", $sformatf("Mismatch TX[%0d]=0x%0h vs RX[%0d]=0x%0h", tx_t.seq_number, tx_t.data, rx_t.seq_number, rx_t.data));
    //                     before_list.delete(i);
    //                     break;
    //                 end
    //             end

    //             if (!matched)
    //                 `uvm_info("UART_SCB", $sformatf("RX[%0d]=0x%0h unmatched", rx_t.seq_number, rx_t.data), UVM_LOW)
    //         end
    //     end
    // endtask



    // offset based
    task compare();
        // Algorithm:
        // 1. Compute offset between BEFORE and AFTER queues.
        // 2. On RESET, log and wait; update offset only when a matching RX is found. (to ignore early init)
        // 3. For each RX try match with current offset. (regular case).
        // 4. If no match - traverse BEFORE list to find first matching data and update offset. (to handle indetermism introduced by rst_txns).
        // 5. Delete matched TX from BEFORE list. Log unmatched RX.
        // This will give good results provided that the data in tx are ranodm enough,
        // and self heal upon mismatched to prevent offset calculation errors forward.

        uart_txn txn;
        uart_rst_txn rst_t;
        uart_rx_txn  rx_t;
        uart_tx_txn  tx_t;
        bit matched;
        int i;
        int offset = 0;

        forever begin
            after_fifo.get(txn);

            if ($cast(rst_t, txn)) begin
                reset_count++;
                `uvm_info("SCB_AFTER", $sformatf("RESET observed, before_list size=%0d", before_list.size()), UVM_LOW)
            end
            else if ($cast(rx_t, txn)) begin
                matched = 0;

                for (i = 0; i < before_list.size(); i++) begin
                    tx_t = before_list[i];
                    if ((rx_t.seq_number + offset == tx_t.seq_number) && (rx_t.data == tx_t.data)) begin
                        matched = 1;
                        matched_count++;
                        `uvm_info("UART_SCB", $sformatf("TX[%0d] matches RX[%0d]=0x%0h (offset=%0d)", tx_t.seq_number, rx_t.seq_number, rx_t.data, offset), UVM_LOW)
                        before_list.delete(i);
                        break;
                    end
                end

                if (!matched) begin
                    for (i = 0; i < before_list.size(); i++) begin
                        tx_t = before_list[i];
                        if (tx_t.data == rx_t.data) begin
                            offset = tx_t.seq_number - rx_t.seq_number;
                            matched = 1;
                            matched_count++;
                            `uvm_info("UART_SCB", $sformatf("Offset updated: %0d (RX seq=%0d, TX seq=%0d)", offset, rx_t.seq_number, tx_t.seq_number), UVM_LOW)
                            before_list.delete(i);
                            break;
                        end
                    end
                end

                if (!matched) begin
                    mismatch_count++;
                    `uvm_error("UART_SCB", $sformatf("RX[%0d]=0x%0h unmatched (offset=%0d)", rx_t.seq_number, rx_t.data, offset))
                end
            end
        end
    endtask

    function void check_phase(uvm_phase phase);
        super.check_phase(phase);
        `uvm_info("UART_SCB",
                  $sformatf("summary: matched=%0d mismatched=%0d resets=%0d pending=%0d",
                            matched_count, mismatch_count, reset_count,
                            before_list.size()),
                  UVM_LOW)
        if (matched_count == 0)
            `uvm_fatal("UART_SCB", "No UART TX/RX transaction was compared")
        if (mismatch_count != 0)
            `uvm_fatal("UART_SCB", $sformatf("%0d UART comparisons failed", mismatch_count))
    endfunction
endclass
