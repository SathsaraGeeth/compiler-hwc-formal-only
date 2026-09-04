/* Maps compiler proof modes to Pono and preserves the complete proof log. */

#include "pono.h"
#include "process.h"
#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace emul::formal {
namespace {
std::string engine(EngineKind kind) {
    if (kind == EngineKind::bmc) return "bmc";
    if (kind == EngineKind::k_induction) return "ind";
    if (kind == EngineKind::pdr) return "ic3bits";
    throw std::runtime_error("Pono requires BMC, k-induction, or PDR");
}

std::string pono_name(
    const std::filesystem::path& transition_system,
    std::string name) {
    auto negated = !name.empty() && name.front() == '~';
    auto symbol = negated ? name.substr(1) : name;
    std::ifstream input(transition_system);
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream fields(line);
        std::string id;
        std::string opcode;
        std::string sort;
        std::string candidate;
        fields >> id >> opcode >> sort >> candidate;
        if ((opcode == "input" || opcode == "state") &&
            candidate == symbol)
            return (negated ? "~" : "") + opcode + id;
    }
    throw std::runtime_error("BTOR2 signal not found: " + symbol);
}

uint32_t justice_conditions(
    const std::filesystem::path& transition_system) {
    std::ifstream input(transition_system);
    std::string id;
    std::string opcode;
    while (input >> id >> opcode) {
        if (opcode == "justice") {
            uint32_t count = 0;
            input >> count;
            return count;
        }
        std::string remainder;
        std::getline(input, remainder);
    }
    return 0;
}
}

Report run_pono(
    const std::filesystem::path& executable,
    const std::filesystem::path& transition_system,
    const EngineOptions& options,
    std::ostream& log) {
    // Safety violations may be transient. Sampling only the final requested
    // bound can miss a bad state that occurs at an earlier cycle and then
    // disappears. Check every BMC bound; Pono still stops at the first
    // counterexample.
    auto step = uint32_t{1};
    auto effective_depth = options.kind == EngineKind::bmc ?
        ((options.depth + step - 1) / step) * step : options.depth;
    auto justice_count = justice_conditions(transition_system);
    auto justice = justice_count != 0;
    std::vector<std::string> arguments = {
        "--engine", engine(options.kind),
        "--bound", std::to_string(effective_depth),
        "--static-coi",
        "--verbosity", "1"
    };
    if (!justice)
        arguments.push_back("--witness");
    if (justice)
        arguments.insert(arguments.end(), {
            "--justice", "--justice-translator",
            justice_count == 1 ? "klive" : "l2s"
        });
    if (options.kind == EngineKind::bmc) {
        arguments.insert(arguments.end(), {
            "--bmc-bound-step", std::to_string(step),
            "--bmc-allow-non-minimal-cex"
        });
    }
    // Pono's --clock switch models one globally toggling clock. For multiple
    // domains the BTOR2 transition system's independent edge detectors must
    // remain authoritative, so do not incorrectly nominate one global clock.
    if (options.clocks.size() <= 1 && !options.clock.empty())
        arguments.insert(arguments.end(), {
            "--clock", pono_name(transition_system, options.clock)
        });
    if (!options.reset.empty())
        arguments.insert(arguments.end(), {
            "--reset", pono_name(transition_system, options.reset),
            "--resetsteps", std::to_string(options.reset_steps)
        });
    arguments.push_back(transition_system.string());
    auto result = run_process(executable, arguments, {.timeout = options.timeout});
    log << result.output;
    if (result.status == 127)
        throw std::runtime_error(
            "cannot execute Pono: " + executable.string());
    Report report;
    report.depth = effective_depth;
    report.counterexample = result.output;
    report.elapsed = result.elapsed;
    report.backend = "pono";
    report.witness = decode_btor_witness(transition_system, result.output);
    if (result.timed_out) report.result = Result::timeout;
    else if (result.status == 0) report.result = Result::counterexample;
    else if (result.status == 1) report.result = Result::proved;
    else if (options.kind == EngineKind::bmc && result.status == 255)
        report.result = Result::bounded;
    else report.result = Result::unknown;
    return report;
}
}
