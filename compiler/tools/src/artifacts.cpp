/*
 * compiler/tools/src/artifacts.cpp
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

#include "artifacts.h"
#include "input.h"
#include "IR/Verifier.h"
#include "Conversion/EIRToIR.h"
#include "IR/Printer.h"
#include "IRGen/Frontend/Lowering.h"
#include "VIR/Printer.h"
#include "VIR/Verifier.h"
#include "eir/include/IR/Printer.h"
#include "eir/include/IR/Verifier.h"
#include "eir/include/Lowering/Frontend.h"
#include "frontend/frontend.h"
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace emul::tool {
namespace {
std::ofstream output(const std::filesystem::path& path) {
    std::ofstream stream(path);
    if (!stream) throw std::runtime_error("cannot write artifact: " + path.string());
    return stream;
}
}

void write_artifacts(const Session& session,
                     const std::filesystem::path& directory) {
    if (session.inputs.sources.empty())
        throw std::runtime_error("no SystemVerilog sources were read");
    if (session.top.empty())
        throw std::runtime_error("set_top must be called before write_artifacts");

    auto design = frontend::Frontend().elaborate(frontend_input(session));
    auto hardware = lowering::to_eir(design);
    eir::verify(hardware);
    auto host = ::vir::irgen::lower_frontend(design, session.uvm_test);
    auto diagnostics = ::vir::Verifier::verify(*host);
    if (!diagnostics.empty())
        throw std::runtime_error(diagnostics.front().message);

    std::filesystem::create_directories(directory);
    auto eir_stream = output(directory / "program.eir");
    eir::print(hardware, eir_stream);
    auto vir_stream = output(directory / "program.vir");
    ::vir::Printer::print(*host, vir_stream);

    std::filesystem::remove(directory / "program.btor2");
    std::optional<btor2::Module> transition_system;
    try {
        transition_system = btor2::lower(hardware);
        btor2::verify(*transition_system);
    } catch (const std::runtime_error& error) {
        std::cerr << "BTOR2 not emitted: " << error.what() << '\n';
    }
    if (transition_system) {
        auto btor2_stream = output(directory / "program.btor2");
        btor2::print(*transition_system, btor2_stream);
    }
}
}
