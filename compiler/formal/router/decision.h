#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "Lowering/Formal/PropertyProfile.h"
#include "frontend/elaborated_design.h"

namespace emul::formal::router {

enum class TaskKind {
    automatic,
    safety,
    liveness,
    fairness,
    reachability,
    performance
};

enum class EngineKind { automatic, smt, bmc, k_induction, pdr };

struct Request {
    btor2::FormalPropertyFeatures features;
    TaskKind task_override = TaskKind::automatic;
    EngineKind engine_override = EngineKind::automatic;
};

struct Decision {
    TaskKind task = TaskKind::automatic;
    EngineKind engine = EngineKind::automatic;
    bool manually_selected = false;
    std::string reason;
};

TaskKind parse_task(std::string_view name);
EngineKind parse_engine(std::string_view name);
std::string_view name(TaskKind kind);
std::string_view name(EngineKind kind);
Decision route(const Request& request);
Decision select(const frontend::ElaboratedDesign&, std::string_view top,
                std::string_view property, std::string_view task_override,
                std::string_view engine_override, std::uint32_t bound);

} // namespace emul::formal::router
