/*
 * compiler/btor2/include/Lowering/TwoStateEIRLowering.h
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
 * Converts an EIR program into a two state BTOR2 transition system
 */

#pragma once

#include "IR/Builder.h"
#include "eir/include/IR/Module.h"
#include <cstddef>
#include <string>
#include <unordered_map>

namespace emul::btor2 {
struct TwoStateTransitionSystem {
    Builder builder;
    std::unordered_map<std::string, Btor2Value> signals;
    size_t next_auxiliary = 0;
};

TwoStateTransitionSystem lower_two_state(const eir::Program& program);
}
