/*
 * compiler/btor2/include/Lowering/Formal/lower_expression.h
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
 * Converts sampled SVA expressions into BTOR2 values
 */

#pragma once
#include "system.h"
#include "frontend/SVA/Model/expression.h"

namespace emul::formal {
Btor2Value lower_sva_expression(
    const frontend::sva::Expression& expression,
    TransitionSystem& system,
    Btor2Value sample = {});
Btor2Value as_boolean(Btor2Value value, TransitionSystem& system);
}
