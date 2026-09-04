/*
 * compiler/frontend/include/frontend/SVA/Lowering/property.h
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

#pragma once
#include "formal/solver/solver.h"
#include "frontend/elaborated_design.h"
#include <string_view>

namespace emul::frontend::sva {
formal::Result lower_and_prove(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    formal::Solver& solver);
}
