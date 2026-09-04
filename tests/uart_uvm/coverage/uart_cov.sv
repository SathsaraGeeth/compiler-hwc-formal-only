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

class uart_cov extends uvm_subscriber #(uart_txn);
    `uvm_component_utils(uart_cov)

    bit rst_val;
    bit [7:0]    tx_data, rx_data;
    int unsigned tx_seq, rx_seq;

    covergroup tx_cg;
    cp_tx_data: coverpoint tx_data {
        bins zero  = {8'h00};
        bins low   = {[8'h01:8'h1F]};
        bins mid   = {[8'h20:8'h7F]};
        bins high  = {[8'h80:8'hFE]};
        bins ff    = {8'hFF};
    }
    cp_tx_seq: coverpoint tx_seq;
    endgroup
    covergroup rx_cg;
    cp_rx_data: coverpoint rx_data {
        bins zero  = {8'h00};
        bins low   = {[8'h01:8'h1F]};
        bins mid   = {[8'h20:8'h7F]};
        bins high  = {[8'h80:8'hFE]};
        bins ff    = {8'hFF};
    }
    cp_rx_seq: coverpoint rx_seq;
    endgroup
    covergroup rst_cg;
    cp_rst: coverpoint rst_val {
        bins asserted   = {1'b1};
    }
    endgroup

    function new(string name = "uart_cov", uvm_component parent = null);
    super.new(name, parent);
    tx_cg  = new();
    rx_cg  = new();
    rst_cg = new();
    endfunction

    function void write(uart_txn t);
    uart_rst_txn rst_t;
    uart_tx_txn  tx_t;
    uart_rx_txn  rx_t;

    if ($cast(rst_t, t)) begin
        rst_val = 1'b1;
        rst_cg.sample();
    end else if ($cast(tx_t, t)) begin
        tx_data = tx_t.data;
        tx_seq  = tx_t.seq_number;
        tx_cg.sample();
    end else if ($cast(rx_t, t)) begin
        rx_data = rx_t.data;
        rx_seq  = rx_t.seq_number;
        rx_cg.sample();
    end
    endfunction
endclass
