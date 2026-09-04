/*
 * compiler/btor2/include/Lowering/Formal/history.h
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
 * Builds sampled history state for SVA expressions
 */

#pragma once
#include "system.h"

namespace emul::formal {

class SampledHistory {
public:
    static Btor2Value previous(
        Btor2Value value,
        uint32_t ticks,
        TransitionSystem& system,
        Btor2Value sample = {});
};

}
