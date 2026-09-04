#include "Runtime/IntrinsicRuntime.h"
#include "ABI/UVM.h"
#include <array>
#include <stdexcept>
#include <vector>

namespace {
class MockTransport final : public transport::Transport {
public:
    transport::Status try_schedule_job(transport::Job& job) override {
        last_job = job.id;
        last_instance = job.instance;
        if (!retried) { retried = true; return transport::Status::RETRY; }
        return transport::Status::SUCCESS;
    }
    transport::Status try_export_signal(
        transport::Signal signal, const transport::Value& value) override {
        last_signal = signal; last_value = value;
        return transport::Status::SUCCESS;
    }
    transport::Status try_import_signal(
        transport::Signal signal, transport::Value& value, bool peek) override {
        last_signal = signal; last_peek = peek; value = last_value;
        return transport::Status::SUCCESS;
    }
    bool retried = false, last_peek = false;
    uint64_t last_job = 0;
    const char* last_instance = nullptr;
    transport::Signal last_signal = 0;
    transport::Value last_value{};
};
}

int main() {
    MockTransport platform;
    vir::runtime::IntrinsicRuntime runtime(platform);

    auto job = runtime.evaluate_dut("@adder_1001");
    if (job->execute() != transport::Status::RETRY) return 1;
    auto scheduled = job->execute();
    if (scheduled != transport::Status::SUCCESS || job->job().id != 1 ||
        std::string_view(platform.last_instance) != "@adder_1001") return 2;

    transport::Value signal{7, 0, 0, 8};
    if (runtime.signals().export_signal(9, signal) != transport::Status::SUCCESS) return 3;
    transport::Value imported{};
    if (runtime.signals().import_signal(9, imported, true) != transport::Status::SUCCESS ||
        imported.data != 7 || !platform.last_peek) return 4;

    if (!runtime.externals().add("identity", [](std::span<const vir::runtime::Value> args) {
            if (args.size() != 1) return vir::runtime::Result<vir::runtime::Value>::failed("arity");
            return vir::runtime::Result<vir::runtime::Value>::completed(args.front());
        })) return 5;
    std::array<vir::runtime::Value, 1> arguments{uint64_t{42}};
    auto external = runtime.externals().call("identity", arguments);
    if (!external.ready() || std::get<uint64_t>(external.value()) != 42) return 6;

    auto object = runtime.objects().create("packet");
    if (!object || !runtime.objects().store(object, "data", uint64_t{11}).ready()) return 7;
    auto field = runtime.objects().load(object, "data");
    if (!field.ready() || std::get<uint64_t>(field.value()) != 11) return 8;
    if (!runtime.objects().add_method("packet", "data", [&runtime](auto self, auto) {
            return runtime.objects().load(self, "data");
        })) return 9;
    auto method = runtime.objects().call(object, "data", {});
    if (!method.ready() || std::get<uint64_t>(method.value()) != 11) return 10;

    auto component = runtime.uvm().factory().create("uart_driver", "driver");
    if (!component.ready()) return 11;
    auto component_name = runtime.objects().load(component.value(), "name");
    if (!component_name.ready() ||
        std::get<std::string>(component_name.value()) != "driver") return 12;

    runtime.uvm().config().set("uvm_test_top.*", "vif", uint64_t{17});
    auto config = runtime.uvm().config().get("uvm_test_top.env", "vif");
    if (!config.ready() || std::get<uint64_t>(config.value()) != 17) return 13;

    runtime.uvm().sequences().start_item("uart", vir::runtime::ObjectHandle{99});
    auto item = runtime.uvm().sequences().try_next_item("uart");
    if (!item || std::get<vir::runtime::ObjectHandle>(*item).value != 99) return 14;

    runtime.uvm().coverage().register_bin("uart.tx.zero");
    runtime.uvm().coverage().register_bin("uart.tx.other");
    runtime.uvm().coverage().sample("uart.tx.zero", uint64_t{0});
    if (runtime.uvm().coverage().percentage() != 50.0) return 15;

    std::vector<std::string> phases;
    auto root = runtime.objects().create("uart_test");
    vir::runtime::ObjectHandle env;
    runtime.uvm().components().add(root, "uart_test", "uvm_test_top");
    runtime.uvm().components().register_type(
        "uart_test", "build", [&](auto parent) {
            phases.push_back("test.build");
            env = runtime.objects().create("uart_env");
            runtime.uvm().components().add(env, "uart_env", "env", parent);
        });
    runtime.uvm().components().register_type(
        "uart_env", "build", [&](auto) { phases.push_back("env.build"); });
    runtime.uvm().components().register_type(
        "uart_test", "connect", [&](auto) { phases.push_back("test.connect"); });
    runtime.uvm().components().register_type(
        "uart_env", "connect", [&](auto) { phases.push_back("env.connect"); });
    runtime.uvm().components().run("build");
    runtime.uvm().components().run("connect");
    if (phases != std::vector<std::string>{"test.build", "env.build",
                                           "test.connect", "env.connect"}) return 16;
    if (runtime.uvm().components().path(env) != "uvm_test_top.env") return 17;
    std::uint64_t received = 0;
    runtime.uvm().tlm().subscribe("scoreboard.before", [&](const auto& value) {
        received = std::get<std::uint64_t>(value);
    });
    runtime.uvm().tlm().connect("monitor.before", "scoreboard.before");
    runtime.uvm().tlm().write("monitor.before", std::uint64_t{55});
    if (received != 55) return 18;
    runtime.uvm().tlm().fifo_push("before_fifo",
                                  vir::runtime::ObjectHandle{88});
    auto fifo_item = runtime.uvm().tlm().fifo_try_get("before_fifo");
    if (!fifo_item || std::get<vir::runtime::ObjectHandle>(*fifo_item).value != 88 ||
        runtime.uvm().tlm().fifo_try_get("before_fifo")) return 34;

    std::uint64_t process_steps = 0;
    vir::runtime::ObjectHandle process;
    process = runtime.uvm().processes().create(root, [&](auto self) {
        ++process_steps;
        if (runtime.uvm().processes().pc(self) == 0) {
            runtime.uvm().processes().store(self, "transaction", std::uint64_t{73});
            runtime.uvm().processes().set_pc(self, 1);
            runtime.uvm().processes().wait_time(self, 5);
            return;
        }
        if (runtime.uvm().processes().pc(self) == 2) {
            runtime.uvm().processes().complete(self);
            return;
        }
        auto restored = runtime.uvm().processes().load(self, "transaction");
        if (std::get<std::uint64_t>(restored) != 73)
            throw std::runtime_error("process local was not restored");
        runtime.uvm().processes().set_pc(self, 2);
        runtime.uvm().processes().wait_event(self, "uart.rx");
    });
    runtime.uvm().processes().run();
    runtime.uvm().processes().advance(4);
    runtime.uvm().processes().run();
    if (process_steps != 1) return 27;
    runtime.uvm().processes().advance(5);
    runtime.uvm().processes().run();
    if (process_steps != 2 || runtime.uvm().processes().pc(process) != 2)
        return 28;
    runtime.uvm().processes().notify("other");
    runtime.uvm().processes().run();
    if (process_steps != 2) return 29;
    runtime.uvm().processes().notify("uart.rx");
    runtime.uvm().processes().run();
    if (process_steps != 3) return 30;
    auto drained = runtime.uvm().processes().create(root, [&](auto self) {
        if (runtime.uvm().processes().pc(self) == 0) {
            runtime.uvm().processes().set_pc(self, 1);
            runtime.uvm().processes().wait_time(self, 3);
        } else {
            runtime.uvm().processes().complete(self);
        }
    });
    if (!runtime.uvm().processes().drain() ||
        runtime.uvm().processes().status(drained) !=
            vir::runtime::builtin::uvm::Process::Status::done)
        return 31;
    runtime.uvm().collections().push(root, "pending",
                                     vir::runtime::ObjectHandle{101});
    runtime.uvm().collections().push(root, "pending",
                                     vir::runtime::ObjectHandle{102});
    if (runtime.uvm().collections().size(root, "pending") != 2 ||
        std::get<vir::runtime::ObjectHandle>(
            runtime.uvm().collections().get(root, "pending", 1)).value != 102)
        return 32;
    runtime.uvm().collections().erase(root, "pending", 0);
    if (runtime.uvm().collections().size(root, "pending") != 1 ||
        std::get<vir::runtime::ObjectHandle>(
            runtime.uvm().collections().get(root, "pending", 0)).value != 102)
        return 33;

    vir_uvm_reset();
    auto abi_object = vir_uvm_factory_create("uart_item", "item", 0);
    if (!abi_object) return 19;
    vir_uvm_config_set_u64("uvm_test_top.*", "vif", 23);
    std::uint64_t abi_value = 0;
    if (!vir_uvm_config_get_u64("uvm_test_top.env", "vif", &abi_value) ||
        abi_value != 23) return 20;
    vir_uvm_sequence_start_u64("uart", abi_object);
    if (!vir_uvm_sequence_try_next_u64("uart", &abi_value) ||
        abi_value != abi_object) return 21;
    vir_uvm_coverage_register("uart.rx");
    vir_uvm_coverage_sample_u64("uart.rx", 1);
    if (vir_uvm_coverage_percentage() != 100.0) return 22;
    vir_uvm_reset();
    if (vir_uvm_coverage_percentage() != 100.0) return 23;
    vir_uvm_report(0, "TEST", "information", 100, "intrinsics.cpp", 1);
    vir_uvm_report(1, "TEST", "warning", 0, "intrinsics.cpp", 2);
    vir_uvm_report(2, "TEST", "error", 0, "intrinsics.cpp", 3);
    if (vir_uvm_error_count() != 1) return 24;
    vir_uvm_reset();
    try {
        vir_uvm_report(3, "TEST", "fatal", 0, "intrinsics.cpp", 4);
        return 25;
    } catch (const std::runtime_error&) {
        if (vir_uvm_error_count() != 1) return 26;
    }
    vir_uvm_reset();
    return 0;
}
