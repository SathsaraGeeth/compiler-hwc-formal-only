/*
 * compiler/btor2/lib/Transform/IR/InstructionCombining.cpp
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

#include "Transform/IR/InstructionCombining.h"
#include "Transform/IR/AlgebraicSimplification.h"
#include "Transform/IR/BranchSimplification.h"
#include "Transform/IR/PeepholeOptimization.h"

namespace emul::btor2::transform {
bool combine_instructions(Module& module) {
    bool changed = false;
    changed |= simplify_algebra(module);
    changed |= simplify_branches(module);
    changed |= optimize_peepholes(module);
    return changed;
}
}
