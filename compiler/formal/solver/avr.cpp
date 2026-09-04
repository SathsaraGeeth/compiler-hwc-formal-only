#include "avr.h"
#include "process.h"
#include <cstdlib>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace emul::formal {
Report run_avr(const std::filesystem::path& executable,
               const std::filesystem::path& transition_system,
               const EngineOptions& options, std::ostream& log) {
    if (options.kind != EngineKind::pdr)
        throw std::runtime_error("AVR is selected only for unbounded safety proofs");
    if (options.clocks.size() > 1)
        throw std::runtime_error(
            "AVR accepts only one global clock; use BtorMC for bounded "
            "multiclock checking or Pono with explicit BTOR2 clock edges");
    auto pattern = (std::filesystem::temp_directory_path() /
                    "hwc-avr-XXXXXX").string();
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    auto directory_text = mkdtemp(buffer.data());
    if (!directory_text)
        throw std::runtime_error("cannot create AVR working directory");
    const std::filesystem::path directory(directory_text);
    const auto root = executable.parent_path();
    const auto bin = root / "build/bin";
    const auto model_dir = transition_system.parent_path();
    const auto clock = options.clock.empty() ? "clk" : options.clock;
    const std::vector<std::string> arguments = {
        transition_system.filename().string(), "-", model_dir.string(), "hwc",
        directory.string(), bin.string(), "-", clock, "3590", "118000",
        "False", "False", "2", "False", "0", "-", "0", "-", "False",
        "sa+uf", "False", "0", "0", "2", "0", "-", "False", "False",
        "0000000", "True", "False", "False", std::to_string(options.depth),
        "False", "False", "y2bt"
    };
    auto process = run_process(executable, arguments, {.timeout = options.timeout});
    log << process.output;
    if (process.status == 127) {
        std::filesystem::remove_all(directory);
        throw std::runtime_error("cannot execute AVR: " + executable.string());
    }
    std::ifstream input(directory / "work_hwc/result.pr");
    std::string result;
    std::getline(input, result);
    Report report;
    report.depth = options.depth;
    report.counterexample = process.output;
    report.elapsed = process.elapsed;
    report.backend = "avr";
    auto witness_file = directory / "work_hwc/cex.witness";
    if (std::filesystem::exists(witness_file)) {
        std::ifstream witness_input(witness_file);
        std::string witness_text((std::istreambuf_iterator<char>(witness_input)), {});
        report.witness = decode_btor_witness(transition_system, witness_text);
    }
    if (process.timed_out)
        report.result = Result::timeout;
    else if (process.status != 0 || result.empty())
        report.result = Result::unknown;
    else if (result.rfind("avr-h", 0) == 0)
        report.result = Result::proved;
    else if (result.rfind("avr-v", 0) == 0)
        report.result = Result::counterexample;
    else
        report.result = Result::unknown;
    std::filesystem::remove_all(directory);
    return report;
}
}
