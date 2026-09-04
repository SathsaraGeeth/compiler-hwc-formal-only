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

class uart_txn extends uvm_sequence_item;  
  `uvm_object_utils_begin(uart_txn)
  `uvm_object_utils_end
  
  function new(string name = "uart_txn");
    super.new(name);
  endfunction
endclass: uart_txn

class uart_rst_txn extends uart_txn;
  `uvm_object_utils_begin(uart_rst_txn)
  `uvm_object_utils_end

  function new(string name = "uart_rst_txn");
    super.new(name);
  endfunction
endclass: uart_rst_txn

class uart_tx_txn extends uart_txn;
  rand bit [7:0] data;
  int unsigned   seq_number;

  `uvm_object_utils_begin(uart_tx_txn)
    `uvm_field_int(data, UVM_ALL_ON)
    `uvm_field_int(seq_number, UVM_ALL_ON)
  `uvm_object_utils_end

  function new(string name = "uart_tx_txn");
    super.new(name);
  endfunction
endclass: uart_tx_txn


class uart_rx_txn extends uart_txn;
  bit [7:0]    data;
  int unsigned seq_number;

  `uvm_object_utils_begin(uart_rx_txn)
    `uvm_field_int(data, UVM_ALL_ON)
    `uvm_field_int(seq_number, UVM_ALL_ON)
  `uvm_object_utils_end

  function new(string name = "uart_rx_txn");
    super.new(name);
  endfunction
endclass: uart_rx_txn
