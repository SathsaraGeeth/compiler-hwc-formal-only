#include "vir_/runtime/uvm/database/database.h"
#include "vir_/runtime/uvm/component/component.h"
#include "vir_/runtime/uvm/callback/callback.h"
#include "vir_/runtime/uvm/coverage/coverage.h"
#include "vir_/runtime/uvm/event/event.h"
#include "vir_/runtime/uvm/factory/factory.h"
#include "vir_/runtime/uvm/phase/phase.h"
#include "vir_/runtime/uvm/report/report.h"
#include "vir_/runtime/uvm/object/object.h"
#include "vir_/runtime/uvm/randomization/randomization.h"
#include "vir_/runtime/uvm/ral/model.h"
#include "vir_/runtime/uvm/ral/memory.h"
#include "vir_/runtime/vir_runtime/random/random.h"
#include "vir_/runtime/uvm/sequence/sequence.h"
#include "vir_/runtime/uvm/tlm/tlm.h"
#include <sstream>
#include <stdexcept>
#include <unordered_set>

int main() {
    emul::runtime::uvm::Components components;
    components.create("uvm_test_top", "test", "");
    components.create("uvm_test_top.env", "environment", "uvm_test_top");
    if (components.children("uvm_test_top").size() != 1) return 1;

    emul::runtime::uvm::Factory factory;
    factory.override_type("packet", "error_packet");
    if (factory.resolve("packet") != "error_packet") return 2;
    factory.override_instance("packet", "short_packet", "env.agent.req");
    if (factory.resolve("packet", "env.agent.req") != "short_packet") return 2;

    emul::runtime::uvm::Callbacks callbacks;
    callbacks.add("env.agent", "callback0");
    callbacks.add("env.agent", "callback0");
    if (callbacks.registered("env.agent").size() != 1) return 2;
    callbacks.remove("env.agent", "callback0");
    if (!callbacks.registered("env.agent").empty()) return 2;

    emul::runtime::uvm::Database configuration;
    configuration.set("env.agent", "active", "1");
    if (configuration.get("env.agent", "active") != "1") return 3;
    configuration.set("env.*", "timeout", "100");
    if (configuration.get("env.agent.driver", "timeout") != "100") return 3;

    emul::runtime::uvm::Events events;
    events.trigger("reset_done", "cycle=4");
    if (events.wait_trigger("reset_done") != "cycle=4") return 4;

    emul::runtime::uvm::Objects objects;
    auto request = objects.create("request", "req");
    objects.set(request, "address", "42");
    auto request_copy = objects.clone(request);
    if (!objects.compare(request, request_copy) ||
        objects.get(request_copy, "address") != "42") return 4;

    emul::runtime::uvm::Randomization randomization;
    emul::runtime::Random random(7);
    randomization.field("request", "data", 8);
    randomization.range("request", "data", 1, 10);
    for (int attempt = 0; attempt < 32; ++attempt) {
        if (!randomization.randomize(objects, request, random)) return 4;
        auto data = std::stoull(objects.get(request, "data"));
        if (data < 1 || data > 10) return 4;
    }

    emul::runtime::uvm::FunctionalCoverage coverage;
    coverage.coverpoint("request", "data");
    coverage.coverpoint("request", "kind");
    coverage.bin("request", "data", "legal", 1, 10, "Bins");
    coverage.bin("request", "data", "forbidden", 11, 11, "IllegalBins");
    coverage.cross("request", "kind_data", "kind");
    coverage.cross("request", "kind_data", "data");
    objects.set(request, "data", "7");
    objects.set(request, "kind", "2");
    coverage.sample(objects, request);
    coverage.sample(objects, request);
    if (coverage.hits("request", "data", "7") != 2) return 4;
    objects.set(request, "data", "11");
    if (coverage.sample(objects, request).size() != 1) return 4;

    randomization.field("cyclic_request", "tag", 3, true);
    auto cyclic = objects.create("cyclic_request", "cyclic");
    std::unordered_set<uint64_t> tags;
    for (int iteration = 0; iteration < 8; ++iteration) {
        if (!randomization.randomize(objects, cyclic, random)) return 4;
        tags.insert(std::stoull(objects.get(cyclic, "tag")));
    }
    if (tags.size() != 8) return 4;

    randomization.field("related_request", "low", 4);
    randomization.field("related_request", "high", 4);
    randomization.range("related_request", "low", 1, 3);
    randomization.range("related_request", "low", 8, 9);
    randomization.relation("related_request", "high", "GreaterThan",
                           "low", true);
    randomization.relation("related_request", "high", "LessThanEqual",
                           "12", false);
    auto related = objects.create("related_request", "related");
    if (!randomization.randomize(objects, related, random)) return 4;
    auto low = std::stoull(objects.get(related, "low"));
    auto high = std::stoull(objects.get(related, "high"));
    if (!((low >= 1 && low <= 3) || (low >= 8 && low <= 9)) ||
        high <= low || high > 12) return 4;

    randomization.field("advanced_request", "kind", 4);
    randomization.field("advanced_request", "value", 8);
    randomization.field("advanced_request", "mirror", 8);
    randomization.distribution("advanced_request", "kind", 0, 0, 1, true);
    randomization.distribution("advanced_request", "kind", 1, 3, 6, false);
    randomization.conditional_range("advanced_request", "zero", "kind",
        "Equality", "0", false, "value", 1, 4, false);
    randomization.conditional_range("advanced_request", "nonzero", "kind",
        "Inequality", "0", false, "value", 8, 15, false);
    randomization.unique("advanced_request", "values", "value");
    randomization.unique("advanced_request", "values", "mirror");
    randomization.solve_before("advanced_request", "kind", "value");
    auto advanced = objects.create("advanced_request", "advanced");
    for (int iteration = 0; iteration < 32; ++iteration) {
        if (!randomization.randomize(objects, advanced, random)) return 4;
        auto kind = std::stoull(objects.get(advanced, "kind"));
        auto value = std::stoull(objects.get(advanced, "value"));
        auto mirror = std::stoull(objects.get(advanced, "mirror"));
        if (kind > 3 || (kind == 0 && (value < 1 || value > 4)) ||
            (kind != 0 && (value < 8 || value > 15)) || value == mirror)
            return 4;
    }

    emul::runtime::uvm::RegisterModel registers;
    registers.block("register_block", "null");
    registers.reg("control", "register_block");
    registers.field("enable", "control", 1, 0, "RW", 1);
    registers.field("mode", "control", 2, 1, "RW", 2);
    registers.map("bus", "register_block", 0x1000, 4, "little");
    registers.map_reg("bus", "control", 0x20, "RW");
    if (registers.read("control") != 5) return 5;
    registers.write("control", 0x5a);
    if (registers.read("control") != 0x5a) return 5;
    registers.predict("control", 0xa5);
    if (registers.mirror("control", true)) return 5;
    if (registers.read("control") != 0xa5) return 5;
    registers.reset("register_block");
    if (registers.read("control") != 5) return 5;
    registers.set("control", 0x12);
    if (!registers.needs_update("control") ||
        registers.get("control") != 0x12 ||
        registers.mirrored("control") != 5) return 5;
    registers.update("control");
    if (registers.needs_update("control") ||
        registers.mirrored("control") != 0x12) return 5;
    registers.poke("control", 0x34);
    if (registers.get("control") != 0x34 ||
        registers.mirrored("control") != 0x34) return 5;

    registers.reg("status", "register_block");
    registers.field("clear", "status", 1, 0, "W1C", 1);
    registers.field("fixed", "status", 1, 1, "RO", 1);
    registers.field("sticky", "status", 1, 2, "W1S", 0);
    registers.field("event", "status", 1, 3, "RC", 1);
    registers.write("status", 0x5);
    if (registers.read("status") != 0xe ||
        registers.read("status") != 0x6) return 5;

    emul::runtime::uvm::MemoryModel memories;
    memories.define("byte_memory", 16, 8, "RW");
    memories.configure("memory", "register_block", "byte_memory");
    memories.write("memory", 3, 0x15a);
    if (memories.read("memory", 3) != 0x5a) return 5;
    memories.reset("register_block");
    if (memories.read("memory", 3) != 0) return 5;

    emul::runtime::uvm::Tlm tlm;
    tlm.connect("monitor.analysis", "scoreboard.analysis");
    tlm.write("monitor.analysis", "packet0");
    if (tlm.peek("scoreboard.analysis") != "packet0") return 6;
    if (tlm.receive("scoreboard.analysis") != "packet0") return 6;

    emul::runtime::uvm::Sequences sequences;
    sequences.start_item("sequencer", "request0");
    sequences.finish_item("sequencer", "request0");
    if (sequences.get_next_item("sequencer") != "request0") return 7;
    sequences.item_done("sequencer");

    sequences.bind("low_sequence", "priority_sequencer");
    sequences.bind("high_sequence", "priority_sequencer");
    sequences.arbitration("priority_sequencer", 3);
    sequences.start_item("low_sequence", "low", 1);
    sequences.finish_item("low_sequence", "low", 1);
    sequences.start_item("high_sequence", "high", 10);
    sequences.finish_item("high_sequence", "high", 10);
    if (sequences.get_next_item("priority_sequencer") != "high") return 7;
    sequences.item_done("priority_sequencer", "high_response");
    if (!sequences.item_done_ready("high_sequence") ||
        sequences.response("high_sequence") != "high_response") return 7;
    sequences.consume_item_done("high_sequence");
    if (!sequences.lock("low_sequence", false)) return 7;
    if (sequences.lock("high_sequence", true)) return 7;
    sequences.unlock("low_sequence");
    if (!sequences.lock("high_sequence", true)) return 7;
    sequences.unlock("high_sequence");

    emul::runtime::uvm::Phase phase;
    phase.enter("run");
    phase.raise();
    phase.drop();
    phase.exit("run");

    emul::runtime::uvm::ReportServer reports;
    std::ostringstream output;
    reports.emit("uvm_test_top", "info", {"TEST", "started"}, output);
    if (reports.errors() != 0 || output.str() != "UVM_INFO TEST started\n")
        return 8;

    try {
        phase.drop();
        return 9;
    } catch (const std::runtime_error&) {
    }
    return 0;
}
