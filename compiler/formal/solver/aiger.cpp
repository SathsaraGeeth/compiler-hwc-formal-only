#include "aiger.h"
#include "process.h"
#include <fstream>
#include <stdexcept>
#include <string>

namespace emul::formal {
namespace {
bool contains_liveness(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        const auto opcode = line.find("justice", first);
        const auto fair = line.find("fair", first);
        if (opcode != std::string::npos || fair != std::string::npos) return true;
    }
    return false;
}
}

Report run_aiger_backend(const std::filesystem::path& executable,
                         AigerBackend backend,
                         const std::filesystem::path& transition_system,
                         const EngineOptions& options, std::ostream& log) {
    if (options.kind != EngineKind::pdr)
        throw std::runtime_error("ABC PDR, Avy, and SuProve accept unbounded safety tasks only");
    if (contains_liveness(transition_system))
        throw std::runtime_error("this AIGER backend does not accept justice/fairness properties");
    auto directory = options.work_directory / "model";
    std::filesystem::create_directories(directory);
    auto aiger = directory / "bit_level.aig";
    auto conversion = run_process(options.btor2aiger,
                                  {transition_system.string()},
                                  {.timeout = options.timeout});
    if (conversion.timed_out)
        throw std::runtime_error("BTOR2 to AIGER conversion timed out");
    if (conversion.status != 0)
        throw std::runtime_error("BTOR2 to AIGER conversion failed: " + conversion.output);
    { std::ofstream out(aiger, std::ios::binary); out.write(conversion.output.data(), conversion.output.size()); }
    ProcessResult result;
    std::string name;
    if (backend == AigerBackend::abc) {
        name = "abc-pdr";
        result = run_process(executable, {"-c", "read bit_level.aig; pdr"},
                             {.timeout = options.timeout, .working_directory = directory});
    } else if (backend == AigerBackend::avy) {
        name = "avy";
        result = run_process(executable, {"bit_level.aig"},
                             {.timeout = options.timeout, .working_directory = directory});
    } else {
        name = "suprove";
        result = run_process(executable, {"bit_level.aig"},
                             {.timeout = options.timeout, .working_directory = directory});
    }
    log << result.output;
    Report report;
    report.backend = name;
    report.counterexample = result.output;
    report.elapsed = conversion.elapsed + result.elapsed;
    report.depth = options.depth;
    if (conversion.timed_out || result.timed_out) report.result = Result::timeout;
    else if (backend == AigerBackend::avy)
        report.result = result.status == 0 ? Result::proved :
                        result.status == 1 ? Result::counterexample : Result::unknown;
    else if (backend == AigerBackend::suprove && result.output.rfind("0\n", 0) == 0)
        report.result = Result::proved;
    else if (backend == AigerBackend::suprove && result.output.rfind("1\n", 0) == 0)
        report.result = Result::counterexample;
    else if (result.output.find("Property proved") != std::string::npos ||
             result.output.find("Property proved unreachable") != std::string::npos ||
             result.output.find("UNSAT") != std::string::npos)
        report.result = Result::proved;
    else if (result.output.find("of miter") != std::string::npos &&
             result.output.find("was asserted") != std::string::npos ||
             result.output.find("SAT") != std::string::npos)
        report.result = Result::counterexample;
    else report.result = Result::unknown;
    return report;
}
}
