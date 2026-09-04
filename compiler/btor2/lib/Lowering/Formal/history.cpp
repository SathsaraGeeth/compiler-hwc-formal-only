/*
 * compiler/btor2/lib/Lowering/Formal/history.cpp
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

#include "Lowering/Formal/history.h"
#include <stdexcept>

namespace emul::formal {

Btor2Value SampledHistory::previous(
    Btor2Value value,
    uint32_t ticks,
    TransitionSystem& system,
    Btor2Value sample) {
    if (!ticks)
        throw std::runtime_error("sampled-history depth must be positive");
    auto current = value;
    for (uint32_t tick = 0; tick < ticks; ++tick) {
        auto zero = system.builder.constant(
            value.width, std::string(value.width, '0'));
        auto state = system.builder.state(
            value.width, "sva_history_" +
            std::to_string(system.next_auxiliary++));
        system.builder.init(state, zero);
        auto next = sample.node ?
            system.builder.ternary("ite", sample, current, state) : current;
        system.builder.next(state, next);
        current = state;
    }
    return current;
}

}
