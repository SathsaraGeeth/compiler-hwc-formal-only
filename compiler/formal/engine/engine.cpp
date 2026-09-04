/* Coordinates SAT/SMT, BMC, k-induction, and IC3/PDR proof runs. */

#include "engine.h"
#include "../logging/logger.h"
#include "../solver/pono.h"
#include "../solver/btormc.h"
#include "../solver/ric3.h"
#include "../solver/avr.h"
#include "../solver/aiger.h"
#include "../solver/smtbmc.h"
#include "../task/Artifacts.h"
#include "../scheduler/PropertyScheduler.h"
#include "Lowering/Formal/temporary_file.h"
#include "Lowering/Formal/writer.h"
#include "IR/Printer.h"
#include "IR/Verifier.h"
#include "frontend/SVA/Lowering/property.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <future>
#include <sstream>

namespace emul::formal {
namespace {
bool has_justice(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string id;
    std::string opcode;
    while (input >> id >> opcode) {
        if (opcode == "justice" || opcode == "fair") return true;
        std::string remainder;
        std::getline(input, remainder);
    }
    return false;
}

bool prefers_word_level(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream fields(line);
        std::string id;
        std::string opcode;
        std::string kind;
        fields >> id >> opcode >> kind;
        if (opcode != "sort") continue;
        if (kind == "array") return true;
        uint32_t width = 0;
        if (kind == "bitvec" && fields >> width && width >= 32) return true;
    }
    return false;
}

BackendKind select_backend(const EngineOptions& options) {
    if ((options.backend == BackendKind::ric3 ||
         options.backend == BackendKind::avr) && has_justice(options.btor2))
        throw std::runtime_error(
            std::string(backend_name(options.backend)) +
            " supports safety properties only; use Pono for justice/fairness");
    if (options.backend != BackendKind::automatic)
        return options.backend;
    if (options.kind == EngineKind::bmc ||
        options.kind == EngineKind::k_induction)
        return BackendKind::btormc;
    if (options.clocks.size() > 1)
        return BackendKind::pono;
    if (options.kind == EngineKind::pdr && !has_justice(options.btor2))
        return prefers_word_level(options.btor2) ? BackendKind::avr :
                                                   BackendKind::ric3;
    return BackendKind::pono;
}

bool has_state(frontend::SemanticNode node) {
    if (node.kind() == "ProceduralBlock") {
        auto kind = node.text("procedureKind");
        auto body = node.child("body");
        if (kind == "AlwaysFF" || kind == "AlwaysLatch" ||
            (kind == "Always" && body.kind() == "Timed"))
            return true;
    }
    for (auto member : node.children("members"))
        if (has_state(member))
            return true;
    if (node.kind() == "Instance")
        return has_state(node.child("body"));
    return false;
}

bool is_temporal(const frontend::sva::Property& property) {
    using frontend::sva::PropertyKind;
    if (property.kind != PropertyKind::sequence &&
        property.kind != PropertyKind::negation &&
        property.kind != PropertyKind::conjunction &&
        property.kind != PropertyKind::disjunction &&
        property.kind != PropertyKind::iff)
        return true;
    if (property.sequence &&
        property.sequence->kind != frontend::sva::SequenceKind::atom)
        return true;
    return (property.left && is_temporal(*property.left)) ||
           (property.right && is_temporal(*property.right));
}

Report solve_once(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property) {
    Solver solver;
    auto result = frontend::sva::lower_and_prove(
        design, top, property, solver);
    return {result, 0, solver.counterexample()};
}

std::string_view display_result(Result result) {
    if (result == Result::proved) return "PROVED";
    if (result == Result::bounded) return "PASSED_BOUND";
    if (result == Result::covered) return "COVERED";
    if (result == Result::counterexample) return "FAILED";
    if (result == Result::timeout) return "TIMEOUT";
    if (result == Result::error) return "ERROR";
    return "UNKNOWN";
}

Report run_model(const EngineOptions& options, std::ostream& output) {
    auto backend = select_backend(options);
    return backend == BackendKind::btormc ?
        run_btormc(options.btormc, options.btor2, options, output) :
        backend == BackendKind::ric3 ?
        run_ric3(options.ric3, options.btor2, options, output) :
        backend == BackendKind::avr ?
        run_avr(options.avr, options.btor2, options, output) :
        backend == BackendKind::abc ?
        run_aiger_backend(options.abc, AigerBackend::abc,
                          options.btor2, options, output) :
        backend == BackendKind::avy ?
        run_aiger_backend(options.avy, AigerBackend::avy,
                          options.btor2, options, output) :
        backend == BackendKind::suprove ?
        run_aiger_backend(options.suprove, AigerBackend::suprove,
                          options.btor2, options, output) :
        backend == BackendKind::smtbmc ?
        run_smtbmc(options.smtbmc, options.btor2, options, output) :
        run_pono(options.pono, options.btor2, options, output);
}

Result combine(Result safety, Result liveness) {
    if (safety == Result::counterexample ||
        liveness == Result::counterexample)
        return Result::counterexample;
    if (safety == Result::error || liveness == Result::error)
        return Result::error;
    if (safety == Result::timeout || liveness == Result::timeout)
        return Result::timeout;
    if (safety == Result::proved && liveness == Result::proved)
        return Result::proved;
    return Result::unknown;
}
}

EngineKind parse_engine(std::string_view name) {
    if (name == "smt" || name == "sat")
        return EngineKind::smt;
    if (name == "bmc")
        return EngineKind::bmc;
    if (name == "kind" || name == "k-induction")
        return EngineKind::k_induction;
    if (name == "pdr" || name == "ic3")
        return EngineKind::pdr;
    throw std::runtime_error("unknown formal engine: " + std::string(name));
}

std::string_view engine_name(EngineKind kind) noexcept {
    switch (kind) {
    case EngineKind::smt:
        return "SAT/SMT";
    case EngineKind::bmc:
        return "BMC";
    case EngineKind::k_induction:
        return "k-induction";
    case EngineKind::pdr:
        return "IC3/PDR";
    }
    return "unknown";
}

BackendKind parse_backend(std::string_view name) {
    if (name == "auto") return BackendKind::automatic;
    if (name == "pono") return BackendKind::pono;
    if (name == "btormc") return BackendKind::btormc;
    if (name == "ric3") return BackendKind::ric3;
    if (name == "avr") return BackendKind::avr;
    if (name == "abc" || name == "abc-pdr") return BackendKind::abc;
    if (name == "avy") return BackendKind::avy;
    if (name == "suprove") return BackendKind::suprove;
    if (name == "smtbmc") return BackendKind::smtbmc;
    throw std::runtime_error("unknown formal backend: " + std::string(name));
}

std::string_view backend_name(BackendKind kind) noexcept {
    switch (kind) {
    case BackendKind::automatic: return "auto";
    case BackendKind::pono: return "Pono";
    case BackendKind::btormc: return "BtorMC";
    case BackendKind::ric3: return "rIC3";
    case BackendKind::avr: return "AVR";
    case BackendKind::abc: return "ABC PDR";
    case BackendKind::avy: return "Avy";
    case BackendKind::suprove: return "SuProve";
    case BackendKind::smtbmc: return "Yosys SMTBMC";
    }
    return "unknown";
}

Report run(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    const EngineOptions& options,
    std::ostream& output) {
    Logger log(output);
    log.phase("engine", engine_name(options.kind));
    log.phase("property", property);
    if (!options.clocks.empty()) {
        std::string domains;
        for (const auto& clock : options.clocks) {
            if (!domains.empty()) domains += ", ";
            domains += clock;
        }
        log.phase("clock domains", domains);
    }
    if (!options.btor2.empty()) {
        if (options.kind == EngineKind::smt)
            throw std::runtime_error(
                "--btor2 does not support the SMT engine");
        auto backend = select_backend(options);
        log.phase("solver", std::string(backend_name(backend)) + " over BTOR2");
        log.phase("transition system", options.btor2.string());
        auto report = run_model(options, output);
        report.work_directory = options.work_directory;
        write_task_artifacts(report, options, property, options.btor2);
        log.result(display_result(report.result),
                   "depth " + std::to_string(report.depth));
        return report;
    }
    const auto& directive = design.formal_design().directive(top, property);
    if (options.kind != EngineKind::smt) {
        TemporaryFile transition;
        auto model = transition.path();
        if (!options.work_directory.empty()) {
            const auto directory = options.work_directory / "model";
            std::filesystem::create_directories(directory);
            model = directory / "low_opt.btor2";
        }
        auto module = build_transition_system(design, top, property);
        {
            std::ofstream stream(model);
            if (!stream)
                throw std::runtime_error("cannot write transition system: " +
                                         model.string());
            btor2::verify(module);
            btor2::print(module, stream);
        }
        auto generated = options;
        generated.btor2 = model;
        if (generated.kind == EngineKind::pdr &&
            generated.backend == BackendKind::automatic &&
            !generated.work_directory.empty() &&
            has_parallel_property_jobs(module)) {
            auto jobs = create_property_jobs(module, generated.work_directory);
            auto safety = generated;
            safety.btor2 = jobs.safety_model;
            safety.work_directory /= "safety";
            auto liveness = generated;
            liveness.btor2 = jobs.liveness_model;
            liveness.work_directory /= "liveness";
            log.phase("schedule", "safety and liveness partitions in parallel");
            auto launch = [](EngineOptions job) {
                std::ostringstream output;
                auto report = run_model(job, output);
                write_task_artifacts(
                    report, job, job.btor2.stem().string(), job.btor2);
                return std::make_pair(std::move(report), output.str());
            };
            auto safety_future = std::async(
                std::launch::async, launch, std::move(safety));
            auto liveness_future = std::async(
                std::launch::async, launch, std::move(liveness));
            auto [safety_report, safety_output] = safety_future.get();
            auto [liveness_report, liveness_output] = liveness_future.get();
            output << "[formal] safety backend: " << safety_report.backend << '\n'
                   << safety_output
                   << "[formal] liveness backend: " << liveness_report.backend << '\n'
                   << liveness_output;
            Report report;
            report.result = combine(
                safety_report.result, liveness_report.result);
            report.depth = std::max(
                safety_report.depth, liveness_report.depth);
            report.elapsed = std::max(
                safety_report.elapsed, liveness_report.elapsed);
            report.backend = safety_report.backend + "+" +
                             liveness_report.backend;
            if (safety_report.result == Result::counterexample)
                report.witness = std::move(safety_report.witness);
            else if (liveness_report.result == Result::counterexample)
                report.witness = std::move(liveness_report.witness);
            report.work_directory = generated.work_directory;
            write_task_artifacts(report, generated, property, model);
            log.result(display_result(report.result),
                       "depth " + std::to_string(report.depth));
            return report;
        }
        auto backend = select_backend(generated);
        log.phase("solver", std::string(backend_name(backend)) + " over generated BTOR2");
        log.phase("transition system", model.string());
        auto report = run_model(generated, output);
        if (directive.kind == frontend::sva::DirectiveKind::cover_property) {
            report.result = report.result == Result::counterexample ?
                Result::covered : Result::unknown;
        }
        report.work_directory = options.work_directory;
        write_task_artifacts(report, options, property, model);
        log.result(display_result(report.result),
                   "depth " + std::to_string(report.depth));
        return report;
    }
    if (is_temporal(directive.property) || has_state(design.root()))
        throw std::runtime_error(
            "the SMT engine accepts only stateless, non-temporal properties; "
            "use BMC, k-induction, or PDR");
    log.phase("solver", "Z3 bit-vector SMT");

    log.phase("SAT/SMT", "checking negated property");
    auto report = solve_once(design, top, property);
    report.backend = "z3";
    report.work_directory = options.work_directory;
    write_task_artifacts(report, options, property, {});
    log.result(display_result(report.result),
               "depth " + std::to_string(report.depth));
    return report;
}
}
