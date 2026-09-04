/*
 * compiler/btor2/include/Lowering/Formal/monitor.h
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
 * Builds monitor state for clocked temporal SVA obligations
 */

#pragma once
#include "system.h"
#include "frontend/SVA/Model/directive.h"

namespace emul::formal {
Btor2Value lower_sample_event(
    const frontend::sva::Directive& directive,
    TransitionSystem& system);

bool lower_temporal_monitor(
    const frontend::sva::Directive& directive,
    TransitionSystem& system);
}
