/*
 * compiler/btor2/lib/Transform/HIR/PassPipeline.cpp
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

#include "Transform/HIR/PassPipeline.h"
#include "Transform/HIR/ComponentDeduplication.h"
#include "Transform/HIR/PropertySimplification.h"

namespace emul::btor2::transform {
bool optimize(hir::Module& module) {
    bool changed = false;
    changed |= simplify_properties(module);
    changed |= deduplicate_components(module);
    return changed;
}
}
