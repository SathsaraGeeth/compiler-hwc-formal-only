/*
 * compiler/tools/src/native_runtime.cpp
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 *
 */

#include "native_runtime.h"
#include "ABI/Runtime.h"
#include "ABI/UVM.h"
#include "Builtin/UVM/UVM.h"
#include "IO/DescriptorIO.h"
#include "VIR/Printer.h"
#include "emulation/transport/fpga/FpgaTransport.h"
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <set>

namespace emul::tool {
namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        auto pattern = (std::filesystem::temp_directory_path() /
                        "hwc-native-XXXXXX").string();
        storage_.assign(pattern.begin(), pattern.end());
        storage_.push_back('\0');
        auto* created = ::mkdtemp(storage_.data());
        if (!created) throw std::runtime_error("cannot create native build directory");
        path_ = created;
    }
    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }
    const std::filesystem::path& path() const { return path_; }
private:
    std::vector<char> storage_;
    std::filesystem::path path_;
};

void compile(const std::filesystem::path& input,
             const std::filesystem::path& output) {
    auto child = ::fork();
    if (child < 0) throw std::runtime_error("cannot start clang");
    if (!child) {
        ::execlp("clang", "clang", "-shared", "-fPIC", "-O2",
                 "-Wno-override-module",
                 input.c_str(), "-o", output.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    if (::waitpid(child, &status, 0) != child ||
        !WIFEXITED(status) || WEXITSTATUS(status))
        throw std::runtime_error("stock LLVM compilation failed");
}

std::string leaf(std::string name) {
    auto dot = name.rfind('.');
    return dot == name.npos ? name : name.substr(dot + 1);
}

}

bool execute_native(const vir::Module& module, std::string entry,
                    const machine::Program& hardware,
                    std::string instance, transport::Transport& transport) {
    TemporaryDirectory temporary;
    auto llvm = temporary.path() / "host.ll";
    auto library = temporary.path() / "host.so";
    {
        std::ofstream output(llvm);
        vir::Printer::print(module, output);
    }
    compile(llvm, library);
    void* handle = ::dlopen(library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) throw std::runtime_error(::dlerror());
    struct Close { void* value; ~Close() { ::dlclose(value); } } close{handle};

    vir::runtime::abi::clear_design();
    vir_uvm_reset();
    vir::runtime::abi::install_transport(transport);
    struct Reset { ~Reset() { vir::runtime::abi::remove_transport();
                              vir::runtime::abi::clear_design(); } } reset;
    vir::runtime::abi::set_instance(std::move(instance));
    static const std::set<std::string> phases{
        "build_phase", "connect_phase", "run_phase", "check_phase"};
    for (const auto& function : module.functions()) {
        static constexpr std::string_view task_prefix = "__uvm_task.";
        static constexpr std::string_view subscriber_prefix = "__uvm_subscriber.";
        if (function->name().starts_with(subscriber_prefix)) {
            auto* callback = ::dlsym(handle, function->name().c_str());
            if (callback)
                vir_uvm_tlm_subscribe(
                    "analysis_export",
                    reinterpret_cast<vir_uvm_tlm_subscriber>(callback));
            continue;
        }
        if (function->name().starts_with(task_prefix)) {
            auto* callback = ::dlsym(handle, function->name().c_str());
            if (callback)
                vir_uvm_process_register(
                    function->name().substr(task_prefix.size()).c_str(),
                    reinterpret_cast<vir_uvm_process_step>(callback));
            continue;
        }
        auto dot = function->name().rfind('.');
        if (dot == std::string::npos) continue;
        auto phase = function->name().substr(dot + 1);
        if (!phases.contains(phase)) continue;
        auto* callback = ::dlsym(handle, function->name().c_str());
        if (!callback) continue;
        if (phase == "run_phase")
            vir_uvm_component_register_process(
                function->name().substr(0, dot).c_str(),
                reinterpret_cast<vir_uvm_process_step>(callback));
        else
            vir_uvm_component_register(
                function->name().substr(0, dot).c_str(), phase.c_str(),
                reinterpret_cast<vir_uvm_phase_callback>(callback));
    }
    if (auto* coverage_init = ::dlsym(handle, "__uvm_coverage_init"))
        reinterpret_cast<void (*)()>(coverage_init)();
    auto combinational_name = entry.substr(0, entry.rfind('.')) + ".combinational";
    auto* combinational = ::dlsym(handle, combinational_name.c_str());
    if (combinational)
        vir::runtime::abi::set_combinational(
            reinterpret_cast<void (*)()>(combinational));
    auto clocked_name = entry.substr(0, entry.rfind('.')) + ".clocked";
    if (auto* clocked = ::dlsym(handle, clocked_name.c_str()))
        vir::runtime::abi::set_clocked(reinterpret_cast<void (*)()>(clocked));
    for (const auto& binding : hardware.bindings) {
        if (binding.kind == machine::BindingKind::state) continue;
        auto name = leaf(binding.name);
        auto host_name = name;
        bool clock = false;
        for (const auto& mapped : module.signal_bindings()) {
            if (mapped.machine_signal != name) continue;
            host_name = mapped.host_signal;
            clock = mapped.clock;
            break;
        }
        auto* storage = ::dlsym(handle, host_name.c_str());
        if (!storage) continue;
        if (std::getenv("HWC_TRACE_TRANSPORT"))
            std::cerr << "VIR binding machine=" << name
                      << " host=" << host_name
                      << " signal=" << transport::fpga::signal_id(binding.name)
                      << " direction=" << (binding.kind == machine::BindingKind::import
                                                ? "input" : "output") << '\n';
        auto id = transport::fpga::signal_id(binding.name);
        auto input = binding.kind == machine::BindingKind::import;
        if (!vir::runtime::abi::register_signal(
                storage, id, binding.width, input))
            throw std::runtime_error("invalid LLVM signal binding: " + name);
        if (clock) vir::runtime::abi::register_clock(storage);
    }
    auto prefix = entry.substr(0, entry.rfind('.'));
    for (const auto& function : module.functions()) {
        if (function->name() == entry ||
            !function->name().starts_with(prefix + ".initial."))
            continue;
        if (auto* initial = ::dlsym(handle, function->name().c_str()))
            reinterpret_cast<void (*)()>(initial)();
    }
    auto transport_name = prefix + ".transport";
    auto* transport_entry = ::dlsym(handle, transport_name.c_str());
    if (transport_entry) {
        auto run_transport = reinterpret_cast<void (*)()>(transport_entry);
        for (;;) {
            try {
                run_transport();
            } catch (const vir::runtime::io::EndOfInput&) {
                break;
            }
        }
    } else {
        auto* symbol = ::dlsym(handle, entry.c_str());
        if (!symbol) throw std::runtime_error("missing LLVM entry function: " + entry);
        reinterpret_cast<void (*)()>(symbol)();
    }
    if (vir::runtime::builtin::UVM::objection_count())
        throw std::runtime_error("UVM test ended with outstanding objections");
    vir_uvm_coverage_report();
    vir_uvm_report_summary();
    if (vir_uvm_error_count())
        throw std::runtime_error("UVM test completed with errors");
    return true;
}

}
