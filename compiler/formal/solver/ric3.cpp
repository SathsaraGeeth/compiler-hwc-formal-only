#include "ric3.h"
#include "process.h"
#include <ostream>
#include <stdexcept>

namespace emul::formal {
Report run_ric3(const std::filesystem::path& executable,
                const std::filesystem::path& transition_system,
                const EngineOptions& options, std::ostream& log) {
    if (options.kind != EngineKind::pdr)
        throw std::runtime_error("rIC3 is selected only for IC3/PDR safety proofs");
    auto result = run_process(executable, {
        "check", "--ui", "false", "--cex", transition_system.string(), "ic3"
    }, {.timeout = options.timeout});
    log << result.output;
    if (result.status == 127)
        throw std::runtime_error("cannot execute rIC3: " + executable.string());
    Report report;
    report.depth = options.depth;
    report.counterexample = result.output;
    report.elapsed = result.elapsed;
    report.backend = "ric3";
    report.witness = decode_btor_witness(transition_system, result.output);
    if (result.timed_out)
        report.result = Result::timeout;
    else if (result.status != 0)
        report.result = Result::unknown;
    else if (result.output.rfind("UNSAT", 0) == 0)
        report.result = Result::proved;
    else if (result.output.rfind("SAT", 0) == 0)
        report.result = Result::counterexample;
    else
        report.result = Result::unknown;
    return report;
}
}
