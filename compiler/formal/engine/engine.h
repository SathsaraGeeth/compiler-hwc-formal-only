/*
 * Formal proof-engine interface.
 *
 * The engine receives an elaborated design and a named SVA property. Runtime
 * SVA lowering normalizes the property before the selected proof algorithm
 * sends formulas through the solver wrapper.
 */

#pragma once
#include "frontend/elaborated_design.h"
#include "../solver/solver.h"
#include "../witness/Witness.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace emul::formal {
enum class EngineKind {
    smt,
    bmc,
    k_induction,
    pdr
};

enum class BackendKind { automatic, pono, btormc, ric3, avr, abc, avy, suprove, smtbmc };

struct EngineOptions {
    EngineKind kind = EngineKind::smt;
    uint32_t depth = 20;
    // Pono's clocked trace uses separate low and high clock time points.
    // Hold reset through both so the first active clock edge observes reset.
    uint32_t reset_steps = 2;
    std::filesystem::path pono;
    std::filesystem::path btormc;
    std::filesystem::path ric3;
    std::filesystem::path avr;
    std::filesystem::path abc;
    std::filesystem::path avy;
    std::filesystem::path suprove;
    std::filesystem::path btor2aiger;
    std::filesystem::path yosys;
    std::filesystem::path smtbmc;
    std::filesystem::path btor2;
    BackendKind backend = BackendKind::automatic;
    std::string clock;
    std::vector<std::string> clocks;
    std::string reset;
    std::chrono::milliseconds timeout{0};
    std::filesystem::path work_directory;
    std::string timescale = "1ns";
    std::string expected_result = "default";
    bool shortest_cover = true;
};

struct Report {
    Result result = Result::unknown;
    uint32_t depth = 0;
    std::string counterexample;
    std::string backend;
    std::chrono::milliseconds elapsed{0};
    Witness witness;
    std::filesystem::path work_directory;
};

EngineKind parse_engine(std::string_view name);
std::string_view engine_name(EngineKind kind) noexcept;
BackendKind parse_backend(std::string_view name);
std::string_view backend_name(BackendKind kind) noexcept;

Report run(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    const EngineOptions& options,
    std::ostream& log);
}
