/*
 * compiler/btor2/include/Lowering/Formal/LTL/buchi_monitor.h
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
 * Builds a Buchi monitor for a temporal property directive
 */

#pragma once

#include "spot_automaton.h"
#include "Lowering/Formal/system.h"
#include "frontend/SVA/Model/directive.h"

namespace emul::formal::ltl {

class BuchiMonitor {
public:
    void lower(
        const frontend::sva::Directive& directive,
        const Automaton& automaton,
        TransitionSystem& system) const;
};

}
