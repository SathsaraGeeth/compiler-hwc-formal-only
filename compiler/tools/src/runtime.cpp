/*
 * compiler/tools/src/runtime.cpp
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

#include "runtime.h"
#include "input.h"
#include "native_runtime.h"
#include "IRGen/Frontend/Lowering.h"
#include "VIR/Verifier.h"
#include "eir/include/CodeGen/Emit/MachineEmitter.h"
#include "eir/include/IR/Verifier.h"
#include "eir/include/IR/Printer.h"
#include "eir/include/Lowering/Frontend.h"
#include "eir/include/Lowering/LowerToMIR.h"
#include "eir/include/MIR/Verifier.h"
#include "eir/include/MIR/Printer.h"
#include "frontend/frontend.h"
#include "target/emul/emul_target.h"
#include "target/emul/disassembler.h"
#include "emulation/transport/cpu/CpuTransport.h"
#include "emulation/transport/fpga/FpgaTransport.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace emul::tool {
namespace {

struct Compiled {
    frontend::ElaboratedDesign design;
    std::unique_ptr<::vir::Module> host;
    mir::MachineModule machine;
    machine::Program hardware;
};

Compiled compile(const Session& session) {
    if (session.inputs.sources.empty())
        throw std::runtime_error("no SystemVerilog sources were read");
    if (session.top.empty())
        throw std::runtime_error("set_top must be called before running");
    Compiled result;
    result.design = frontend::Frontend().elaborate(frontend_input(session));
    result.host = ::vir::irgen::lower_frontend(result.design, session.uvm_test);
    auto diagnostics = ::vir::Verifier::verify(*result.host);
    if (!diagnostics.empty())
        throw std::runtime_error(diagnostics.front().message);

    auto eir = lowering::to_eir(result.design);
    eir::verify(eir);
    if (std::getenv("HWC_TRACE_EIR")) eir::print(eir, std::cerr);
    if (eir.modules.empty())
        throw std::runtime_error("emulation requires synthesizable hardware");
    result.machine = eir::lowering::lower_to_mir(eir);
    mir::verify(result.machine);
    if (std::getenv("HWC_TRACE_EIR")) mir::print(result.machine, std::cerr);
    result.hardware = machine::encode(
        result.machine, target::emulator::get_target());
    if (std::getenv("HWC_TRACE_EIR"))
        machine::disassemble(result.hardware, std::cerr);
    return result;
}

std::pair<std::string, std::string> fpga_server() {
    const auto* configured = std::getenv("HWC_FPGA_SERVER");
    if (!configured || !*configured)
        throw std::runtime_error(
            "set_target fpga requires HWC_FPGA_SERVER=HOST:PORT");
    std::string server = configured;
    auto separator = server.rfind(':');
    if (separator == server.npos || !separator || separator + 1 == server.size())
        throw std::runtime_error("HWC_FPGA_SERVER must have the form HOST:PORT");
    return {server.substr(0, separator), server.substr(separator + 1)};
}

bool run_native(const Session& session, Compiled& program,
                transport::Transport& transport) {
    auto entry = session.top + ".initial";
    auto instance = program.design.bindings().entries().empty()
        ? program.machine.root().name
        : program.design.bindings().entries().front().hardware.name();
    auto passed = execute_native(*program.host, std::move(entry),
                                 program.hardware, std::move(instance), transport);
    if (passed) std::cout << "PASS\n";
    return passed;
}

}

bool simulate(const Session& session) {
    auto program = compile(session);
    transport::cpu::CpuTransport transport(program.hardware);
    return run_native(session, program, transport);
}

bool emulate(const Session& session) {
    auto program = compile(session);
    if (session.target == Target::cpu) {
        transport::cpu::CpuTransport transport(program.hardware);
        return run_native(session, program, transport);
    }
    auto [host, service] = fpga_server();
    transport::fpga::FpgaTransport transport(std::move(host), std::move(service));
    transport.load(program.hardware);
    return run_native(session, program, transport);
}

}
