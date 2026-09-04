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

// import uvm pkg and macros
`include "uvm_macros.svh"
import uvm_pkg::*;
// import our pkg and macros


// calss(es) for UVM_TESTS passed by +UVM_TESTNAME parameter, in uvm factory without recompiling everytime
class uart_test extends uvm_test; // uvm_components <- uvm_test
    // register this calss in the factory
    `uvm_component_utils(uart_test)

    uart_env u_env;

    // constructor
    function new(string name="uart_test", uvm_component parent=null);
        super.new(name,parent);
    endfunction

    function void build_phase(uvm_phase phase);
        super.build_phase(phase);
        u_env = uart_env::type_id::create("u_env", this);
    endfunction

    // uvm calls run_phase after it creates our object
    task run_phase(uvm_phase phase);
        uart_seq seq;
        real cov;   // coverage

        // uvm objects can raises an objection finishing the test - test run as long as one object has an objection to stop it
        phase.raise_objection(this);    // pass ourselves using this

        seq = uart_seq::type_id::create("seq");
        seq.start(u_env.u_agent.seqr);


        forever begin
            #1ns;
            
            cov = $get_coverage();

            `uvm_info("COV", $sformatf("Coverage: %.2f%%", cov), UVM_LOW)
            
            if(cov >= 80.0) begin
                `uvm_info("COV", "Target coverage reached (80%)", UVM_LOW)
                break;
            end
        end


        phase.drop_objection(this); 
    endtask
endclass : uart_test
