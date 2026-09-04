#include "Analysis/Analysis.h"
#include "Lowering/Formal/PropertyProfile.h"
#include "formal/router/decision.h"
#include <cassert>
#include <stdexcept>

using namespace emul::btor2;
namespace runtime_router = ::emul::formal::router;

int main() {
    emul::btor2::FormalPropertyFeatures safety;
    safety.stateful_design = true;
    auto safety_route = runtime_router::route({safety, {}, {}});
    assert(safety_route.task == runtime_router::TaskKind::safety);
    assert(safety_route.engine == runtime_router::EngineKind::pdr);

    emul::btor2::FormalPropertyFeatures fairness;
    fairness.temporal = true;
    fairness.unbounded_liveness = true;
    fairness.recurrence = true;
    fairness.fairness_implication = true;
    auto fairness_route = runtime_router::route({fairness, {}, {}});
    assert(fairness_route.task == runtime_router::TaskKind::fairness);
    assert(fairness_route.engine == runtime_router::EngineKind::pdr);

    emul::btor2::FormalPropertyFeatures cover;
    cover.cover = true;
    auto cover_route = runtime_router::route({cover, {}, {}});
    assert(cover_route.task == runtime_router::TaskKind::reachability);
    assert(cover_route.engine == runtime_router::EngineKind::bmc);

    bool rejected = false;
    try {
        runtime_router::route({fairness, runtime_router::TaskKind::automatic,
                               runtime_router::EngineKind::smt});
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);

    Module system;
    system.add({"input", 8, {}, {}, "request", {}});
    auto state = system.add({"state", 16, {}, {}, "counter", {}});
    system.states.push_back({state, {}, {}});
    system.properties.push_back({PropertyKind::bad, state, "overflow"});
    auto profile = analyze(system);
    assert(profile.inputs == 1 && profile.input_bits == 8);
    assert(profile.states == 1 && profile.state_bits == 16);
    assert(profile.bad_properties == 1);
    auto execution = plan(profile, 8, 2);
    assert(execution.workers >= 1 && execution.workers <= 2);
}
