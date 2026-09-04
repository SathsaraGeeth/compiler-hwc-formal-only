#include "smtbmc.h"
#include "process.h"
#include <fstream>
#include <stdexcept>

namespace emul::formal {
Report run_smtbmc(const std::filesystem::path& executable,
                  const std::filesystem::path& transition_system,
                  const EngineOptions& options, std::ostream& log) {
    if (options.kind != EngineKind::bmc && options.kind != EngineKind::k_induction)
        throw std::runtime_error("Yosys SMTBMC supports BMC and temporal induction, not IC3/PDR");
    // Yosys' AIGER reader does not reconstruct BTOR2 bad/constraint records as
    // $assert/$assume cells. Running it through that path would report a
    // vacuous PASS. Keep this backend unavailable until the compiler has a
    // native Yosys-SMT2 writer; never silently prove a different model.
    throw std::runtime_error(
        "Yosys SMTBMC needs a native BTOR2-to-Yosys-SMT2 property adapter; "
        "the installed yosys-smtbmc cannot consume BTOR2 directly");
    const auto model_dir = options.work_directory / "model";
    const auto trace_dir = options.work_directory / "traces";
    std::filesystem::create_directories(model_dir);
    std::filesystem::create_directories(trace_dir);
    const auto aiger = model_dir / "bit_level.aig";
    const auto smt2 = model_dir / "model.smt2";
    auto conversion = run_process(options.btor2aiger, {transition_system.string()},
                                  {.timeout = options.timeout});
    if (conversion.timed_out || conversion.status != 0)
        throw std::runtime_error("BTOR2 to AIGER conversion failed: " + conversion.output);
    { std::ofstream out(aiger, std::ios::binary); out.write(conversion.output.data(), conversion.output.size()); }
    auto synthesis = run_process(options.yosys,
        {"-q", "-p", "read_aiger -module_name hwc_model " + aiger.string() +
         "; prep -top hwc_model; write_smt2 -wires " + smt2.string()},
        {.timeout = options.timeout});
    log << synthesis.output;
    if (synthesis.timed_out || synthesis.status != 0)
        throw std::runtime_error("Yosys SMT2 generation failed: " + synthesis.output);
    std::vector<std::string> arguments{"-s", "boolector", "-t", std::to_string(options.depth),
                                       "-m", "hwc_model", "--dump-vcd",
                                       (trace_dir / "smtbmc.vcd").string()};
    if (options.kind == EngineKind::k_induction) arguments.push_back("-i");
    arguments.push_back(smt2.string());
    auto result = run_process(executable, arguments, {.timeout = options.timeout});
    log << result.output;
    Report report;
    report.backend = "yosys-smtbmc";
    report.counterexample = result.output;
    report.elapsed = conversion.elapsed + synthesis.elapsed + result.elapsed;
    report.depth = options.depth;
    if (result.timed_out) report.result = Result::timeout;
    else if (result.output.find("Status: FAILED") != std::string::npos) report.result = Result::counterexample;
    else if (result.output.find("Status: PASSED") != std::string::npos)
        report.result = options.kind == EngineKind::bmc ? Result::bounded : Result::proved;
    else report.result = Result::unknown;
    return report;
}
}
