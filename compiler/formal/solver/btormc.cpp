#include "btormc.h"
#include "process.h"
#include <ostream>
#include <stdexcept>

namespace emul::formal {
Report run_btormc(const std::filesystem::path& executable,
                  const std::filesystem::path& transition_system,
                  const EngineOptions& options, std::ostream& log) {
    if (options.kind != EngineKind::bmc &&
        options.kind != EngineKind::k_induction)
        throw std::runtime_error("BtorMC supports BMC and k-induction only");
    std::vector<std::string> arguments = {
        "--bound-max=" + std::to_string(options.depth), "--trace-gen",
        "--trace-gen-full", "--stop-first"
    };
    if (options.kind == EngineKind::k_induction)
        arguments.push_back("--kind");
    arguments.push_back(transition_system.string());
    auto result = run_process(executable, arguments, {.timeout = options.timeout});
    log << result.output;
    if (result.status == 127)
        throw std::runtime_error("cannot execute BtorMC: " + executable.string());
    Report report;
    report.depth = options.depth;
    report.counterexample = result.output;
    report.elapsed = result.elapsed;
    report.backend = "btormc";
    report.witness = decode_btor_witness(transition_system, result.output);
    if (!report.witness.empty())
        report.depth = report.witness.steps.back().index;
    if (result.timed_out)
        report.result = Result::timeout;
    else if (result.status != 0)
        report.result = Result::unknown;
    else if (result.output.find("sat\n") != std::string::npos)
        report.result = Result::counterexample;
    else if (options.kind == EngineKind::k_induction)
        report.result = Result::proved;
    else
        report.result = Result::bounded;
    return report;
}
}
