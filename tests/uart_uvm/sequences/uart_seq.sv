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

class uart_seq extends uvm_sequence #(uart_txn);
    `uvm_object_utils(uart_seq)

    function new(string name = "uart_seq");
        super.new(name);
    endfunction

    virtual task body();
        uart_txn     txn;
        uart_rst_txn rst;
        uart_tx_txn  tx;

        integer i;
        for (i = 0; i < 1000; i++) begin
            if ($urandom_range(0, 99) < 90) begin
                tx = uart_tx_txn::type_id::create("tx");
                assert(tx.randomize()) else
                    `uvm_error("TX_SEQ", "Randomization failed");
                txn = tx;
            end else begin
                rst = uart_rst_txn::type_id::create("rst");
                txn = rst;
            end
            start_item(txn);
            finish_item(txn);
        end
    endtask: body
endclass: uart_seq

