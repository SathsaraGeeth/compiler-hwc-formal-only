/*
 * compiler/btor2/lib/Analysis/Analysis.cpp
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 *
 */

#include "Analysis/Analysis.h"
#include <algorithm>
#include <thread>

namespace emul::btor2 {

ModelProfile analyze(const Module& module) {
    ModelProfile result;
    result.nodes = module.operations.size();
    result.states = module.states.size();
    result.outputs = module.outputs.size();
    for (const auto& node : module.operations)
        if (node.opcode == "input") {
            ++result.inputs;
            result.input_bits += node.width;
        }
    for (const auto& state : module.states) {
        result.state_bits += module.get(state.value).width;
        result.initialized_states += state.initial.has_value();
        result.next_states += state.next.has_value();
    }
    for (const auto& property : module.properties) {
        if (property.kind == PropertyKind::constraint) ++result.constraints;
        else if (property.kind == PropertyKind::bad) ++result.bad_properties;
        else if (property.kind == PropertyKind::cover) ++result.covers;
        else if (property.kind == PropertyKind::fair) ++result.fairness;
    }
    return result;
}

ExecutionPlan plan(
    const ModelProfile& profile,
    std::uint32_t independent_jobs,
    std::uint32_t requested_workers) {
    if (!independent_jobs) return {};
    auto hardware = std::max(1u, std::thread::hardware_concurrency());
    auto requested = requested_workers ? requested_workers : hardware;
    
    
    auto model_limit = profile.state_bits > 1'000'000 ? 1u :
        profile.state_bits > 250'000 ? std::max(1u, hardware / 4) : hardware;
    ExecutionPlan result;
    result.workers = std::max(1u, std::min({
        requested, hardware, model_limit, independent_jobs}));
    result.parallel = result.workers > 1;
    return result;
}

} 
