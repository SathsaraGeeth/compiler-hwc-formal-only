/*
 * compiler/tools/src/dump.cpp
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

#include "dump.h"
#include "input.h"
#include "IR/Verifier.h"
#include "Conversion/EIRToIR.h"
#include "Conversion/SVAToHIR.h"
#include "HIR/Printer.h"
#include "IR/Printer.h"
#include "IRGen/Frontend/Lowering.h"
#include "Lowering/Formal/writer.h"
#include "Lowering/Formal/harness.h"
#include "Lowering/Formal/PropertyProfile.h"
#include "Transform/HIR/PassPipeline.h"
#include "Transform/IR/PassPipeline.h"
#include "frontend/SVA/Lowering/directive.h"
#include "VIR/Printer.h"
#include "VIR/Verifier.h"
#include "eir/include/CodeGen/Emit/MachineEmitter.h"
#include "eir/include/IR/Printer.h"
#include "eir/include/IR/Verifier.h"
#include "eir/include/Lowering/Flatten.h"
#include "eir/include/Lowering/Frontend.h"
#include "eir/include/Lowering/LowerToMIR.h"
#include "eir/include/MIR/Printer.h"
#include "eir/include/MIR/Verifier.h"
#include "frontend/frontend.h"
#include "target/emul/disassembler.h"
#include "target/emul/emul_target.h"
#include <iostream>
#include <memory>
#include <stdexcept>

namespace emul::tool {
namespace {

frontend::ElaboratedDesign elaborate(const Session& session) {
    if (session.inputs.sources.empty())
        throw std::runtime_error("no SystemVerilog sources were read");
    if (session.top.empty())
        throw std::runtime_error("set_top must be called before dumping an IR");
    return frontend::Frontend().elaborate(frontend_input(session));
}

std::unique_ptr<::vir::Module> vir(
    const frontend::ElaboratedDesign& design,
    std::string_view configured_uvm_test) {
    auto result = ::vir::irgen::lower_frontend(design, configured_uvm_test);
    auto diagnostics = ::vir::Verifier::verify(*result);
    if (!diagnostics.empty()) throw std::runtime_error(diagnostics.front().message);
    return result;
}

eir::Program eir(const frontend::ElaboratedDesign& design) {
    auto result = lowering::to_eir(design);
    eir::verify(result);
    return result;
}
eir::Program eir_before_mir(const frontend::ElaboratedDesign& design) {
    eir::Program result;
    result.modules.push_back(eir::lowering::flatten(eir(design)));
    eir::verify(result);
    return result;
}

}

void dump_representation(const Session& session, std::string_view kind,
                         std::string_view argument) {
    auto design = elaborate(session);
    if (kind == "elb_design") {
        design.print(std::cout);
    } else if (kind == "eir_unopt") {
        eir::print(eir(design), std::cout);
    } else if (kind == "vir_unopt") {
        ::vir::Printer::print(*vir(design, session.uvm_test), std::cout);
    } else if (kind == "eir_opt") {
        eir::print(eir_before_mir(design), std::cout);
    } else if (kind == "vir_opt") {
        ::vir::Printer::print(*vir(design, session.uvm_test), std::cout);
    } else if (kind == "mir") {
        auto machine = eir::lowering::lower_to_mir(eir(design));
        mir::verify(machine);
        mir::print(machine, std::cout);
    } else if (kind == "btor2_high_unopt" || kind == "btor2_high_opt") {
        if (session.formal_property.empty())
            throw std::runtime_error(
                std::string("dump_") + std::string(kind) +
                " needs a selected prove or cover property");
        auto harness = formal::build_harness(
            design, session.top, session.formal_property);
        auto high_level = btor2::convert_sva_to_hir(harness.directives);
        if (kind == "btor2_high_opt")
            btor2::transform::optimize(high_level);
        btor2::hir::print(high_level, std::cout);
    } else if (kind == "btor2_low_unopt" || kind == "btor2_low_opt") {
        if (!session.formal_property.empty()) {
            formal::write_transition_system(
                design, session.top, session.formal_property, std::cout,
                formal::PropertyTarget::all, kind == "btor2_low_opt");
            return;
        }
        auto transition_system = btor2::lower(eir(design));
        if (kind == "btor2_low_opt")
            btor2::transform::optimize(transition_system);
        btor2::verify(transition_system);
        btor2::print(transition_system, std::cout);
    } else if (kind == "asm") {
        if (argument != "backend=emul") {
            if (argument == "backend=cpu")
                throw std::runtime_error(
                    "dump_asm backend=cpu is unavailable: the CPU assembly "
                    "emitter is not implemented");
            throw std::runtime_error("dump_asm expects backend=emul or backend=cpu");
        }
        auto machine = eir::lowering::lower_to_mir(eir(design));
        mir::verify(machine);
        auto program = machine::encode(machine, target::emulator::get_target());
        machine::disassemble(program, std::cout);
    } else {
        throw std::runtime_error("unknown representation dump: " + std::string(kind));
    }
}

}
