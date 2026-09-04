/*
 * compiler/btor2/lib/Transform/IR/PassPipeline.cpp
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

#include "Transform/IR/PassPipeline.h"
#include "Transform/IR/CommonSubexpressionElimination.h"
#include "Transform/IR/ConstantFolding.h"
#include "Transform/IR/CopyPropagation.h"
#include "Transform/IR/DeadCodeElimination.h"
#include "Transform/IR/InstructionCombining.h"
#include "Transform/IR/StrengthReduction.h"

namespace emul::btor2::transform {
bool optimize(Module& module) {
    bool changed = false;
    for (unsigned iteration = 0; iteration != 8; ++iteration) {
        bool local = false;
        local |= fold_constants(module);
        local |= combine_instructions(module);
        local |= propagate_copies(module);
        local |= reduce_strength(module);
        local |= fold_constants(module);
        local |= eliminate_common_subexpressions(module);
        changed |= local;
        if (!local) break;
    }
    changed |= eliminate_dead_code(module);
    return changed;
}
}
