/*
 * compiler/btor2/include/Lowering/Formal/lower_sva.h
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
 * Adds a normalized SVA directive to a BTOR2 transition system
 */

#pragma once
#include "system.h"
#include "frontend/SVA/Model/directive.h"

namespace emul::formal {
void lower_sva_directive(
    const frontend::sva::Directive& directive,
    TransitionSystem& system);
}
