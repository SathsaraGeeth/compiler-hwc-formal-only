#include "vir_/runtime/vir_runtime/executor/executor.h"
#include "vir_/runtime/transport/cpu/host_transport.h"

int main() {
    using emul::vir::Instruction;
    using emul::vir::Process;

    Process top{"top", {
        {{}, "uvm_run_test", "\"blocking_test\""},
        {{}, "return", "0"}
    }};
    Process build{"blocking_test.build_phase", {
        {"%producer", "uvm_component_create",
         "\"producer\", producer, @this"},
        {"%consumer", "uvm_component_create",
         "\"consumer\", consumer, @this"},
        {{}, "return", "0"}
    }};
    Process producer{"producer.run_phase", {
        {{}, "uvm_objection_raise", "@this"},
        {{}, "delay", "2"},
        {"%ready", "const", "1"},
        {{}, "store", "@ready, %ready"},
        {{}, "uvm_tlm_write", "channel, transaction#7"},
        {{}, "uvm_objection_drop", "@this"},
        {{}, "return", "0"}
    }};
    Process consumer{"consumer.run_phase", {
        {{}, "wait", "@ready"},
        {"%item", "uvm_tlm_receive", "channel"},
        {{}, "uvm_report_info", "\"RECEIVED\", %item"},
        {{}, "return", "0"}
    }};

    emul::runtime::HostTransport transport;
    emul::runtime::Executor executor(transport);
    return executor.run({{top, build, producer, consumer}}) ? 0 : 1;
}
