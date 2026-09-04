#include "Artifacts.h"
#include <fstream>
#include <stdexcept>

namespace emul::formal {
namespace {
std::string json(std::string_view value) {
    std::string result;
    for (char character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += "\\n";
        else result += character;
    }
    return result;
}

bool expectation_met(std::string_view expected, Result result) {
    if (expected == "default" || expected == "any") return true;
    return expected == result_name(result) ||
           (expected == "counterexample" && result == Result::counterexample);
}
}

std::string_view result_name(Result result) noexcept {
    switch (result) {
    case Result::proved: return "proved";
    case Result::bounded: return "bounded";
    case Result::covered: return "covered";
    case Result::counterexample: return "counterexample";
    case Result::unknown: return "unknown";
    case Result::timeout: return "timeout";
    case Result::error: return "error";
    }
    return "error";
}

void write_task_artifacts(const Report& report, const EngineOptions& options,
                          std::string_view property,
                          const std::filesystem::path& model) {
    if (options.work_directory.empty()) return;
    const auto backend = options.work_directory / "backend";
    const auto traces = options.work_directory / "traces";
    std::filesystem::create_directories(backend);
    std::filesystem::create_directories(traces);
    {
        std::ofstream output(backend / "output.log");
        output << report.counterexample;
    }
    if (!report.witness.empty()) {
        write_witness_text(report.witness, traces / "witness.txt");
        write_witness_vcd(report.witness, traces / "counterexample.vcd",
                          options.timescale);
    }
    std::ofstream output(options.work_directory / "report.json");
    if (!output) throw std::runtime_error("cannot write formal report");
    output << "{\n"
           << "  \"schema_version\": 1,\n"
           << "  \"property\": \"" << json(property) << "\",\n"
           << "  \"result\": \"" << result_name(report.result) << "\",\n"
           << "  \"backend\": \"" << json(report.backend) << "\",\n"
           << "  \"engine\": \"" << engine_name(options.kind) << "\",\n"
           << "  \"expected_result\": \"" << json(options.expected_result) << "\",\n"
           << "  \"expectation_met\": "
           << (expectation_met(options.expected_result, report.result) ? "true" : "false")
           << ",\n"
           << "  \"depth\": " << report.depth << ",\n"
           << "  \"elapsed_ms\": " << report.elapsed.count() << ",\n"
           << "  \"model\": \"" << json(model.string()) << "\",\n"
           << "  \"witness_steps\": " << report.witness.steps.size() << "\n"
           << "}\n";
}
}
