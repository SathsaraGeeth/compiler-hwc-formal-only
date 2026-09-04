/*
 * compiler/btor2/include/Lowering/Formal/PropertyProfile.h
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
 * Describes the formal features used by a selected property
 */

#pragma once

#include "frontend/elaborated_design.h"
#include <cstdint>
#include <string_view>

namespace emul::btor2 {

struct FormalPropertyFeatures {
    bool cover = false;
    bool stateful_design = true;
    bool temporal = false;
    bool unbounded_liveness = false;
    bool recurrence = false;
    bool fairness_implication = false;
    bool bounded_response = false;
    std::uint32_t bound = 0;
};

FormalPropertyFeatures profile(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    std::uint32_t bound);

} 
