#include "decision.h"
#include <stdexcept>

namespace emul::formal::router {
namespace {
TaskKind classify(const btor2::FormalPropertyFeatures& value) {
    if (value.cover)
        return TaskKind::reachability;
    if (value.fairness_implication)
        return TaskKind::fairness;
    if (value.unbounded_liveness || value.recurrence)
        return TaskKind::liveness;
    if (!value.temporal || value.bounded_response)
        return TaskKind::safety;
    throw std::runtime_error(
        "formal router cannot classify this temporal property safely; "
        "use set_formal_task safety|liveness|fairness|reachability|performance");
}

EngineKind recommended(TaskKind task, const btor2::FormalPropertyFeatures& value) {
    switch (task) {
    case TaskKind::safety:
        return value.stateful_design ? EngineKind::pdr : EngineKind::smt;
    case TaskKind::liveness:
    case TaskKind::fairness:
        return EngineKind::pdr;
    case TaskKind::reachability:
    case TaskKind::performance:
        return EngineKind::bmc;
    case TaskKind::automatic:
        break;
    }
    throw std::runtime_error("formal router produced no task classification");
}

void validate(EngineKind engine, TaskKind task, const btor2::FormalPropertyFeatures& value) {
    if (engine == EngineKind::automatic)
        throw std::runtime_error("formal router produced no engine");
    if (engine == EngineKind::smt && (value.stateful_design || value.temporal))
        throw std::runtime_error(
            "SMT override is invalid for a stateful or temporal property");
    if ((task == TaskKind::liveness || task == TaskKind::fairness) &&
        engine == EngineKind::k_induction)
        throw std::runtime_error(
            "k-induction override cannot prove Büchi liveness/fairness; "
            "use pdr or bmc for bounded diagnostics");
    if (task == TaskKind::performance && !value.bound)
        throw std::runtime_error(
            "performance tasks require a nonzero set_max_depth bound");
}
}

TaskKind parse_task(std::string_view value) {
    if (value == "auto") return TaskKind::automatic;
    if (value == "safety") return TaskKind::safety;
    if (value == "liveness") return TaskKind::liveness;
    if (value == "fairness") return TaskKind::fairness;
    if (value == "reachability") return TaskKind::reachability;
    if (value == "performance") return TaskKind::performance;
    throw std::runtime_error("unknown formal task: " + std::string(value));
}

EngineKind parse_engine(std::string_view value) {
    if (value == "auto") return EngineKind::automatic;
    if (value == "smt" || value == "sat") return EngineKind::smt;
    if (value == "bmc") return EngineKind::bmc;
    if (value == "kind" || value == "k-induction")
        return EngineKind::k_induction;
    if (value == "pdr" || value == "ic3") return EngineKind::pdr;
    throw std::runtime_error("unknown formal engine: " + std::string(value));
}

std::string_view name(TaskKind value) {
    switch (value) {
    case TaskKind::automatic: return "auto";
    case TaskKind::safety: return "safety";
    case TaskKind::liveness: return "liveness";
    case TaskKind::fairness: return "fairness";
    case TaskKind::reachability: return "reachability";
    case TaskKind::performance: return "performance";
    }
    throw std::runtime_error("invalid formal task enum");
}

std::string_view name(EngineKind value) {
    switch (value) {
    case EngineKind::automatic: return "auto";
    case EngineKind::smt: return "smt";
    case EngineKind::bmc: return "bmc";
    case EngineKind::k_induction: return "kind";
    case EngineKind::pdr: return "pdr";
    }
    throw std::runtime_error("invalid formal engine enum");
}

Decision route(const Request& request) {
    Decision result;
    result.task = request.task_override == TaskKind::automatic ?
        classify(request.features) : request.task_override;
    result.engine = request.engine_override == EngineKind::automatic ?
        recommended(result.task, request.features) : request.engine_override;
    result.manually_selected =
        request.task_override != TaskKind::automatic ||
        request.engine_override != EngineKind::automatic;
    validate(result.engine, result.task, request.features);
    result.reason = result.manually_selected ? "validated manual override" :
        result.task == TaskKind::fairness ? "recurrence implication" :
        result.task == TaskKind::liveness ? "unbounded progress operator" :
        result.task == TaskKind::reachability ? "cover directive" :
        result.task == TaskKind::performance ? "bounded performance query" :
        result.engine == EngineKind::smt ? "stateless safety property" :
        "stateful safety property";
    return result;
}

} // namespace emul::formal::router
