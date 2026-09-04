/*
 * compiler/btor2/include/Transform/HIR/PropertySimplification.h
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
 * Simplifies higher level temporal property expressions
 */

#pragma once
#include "HIR/Module.h"

namespace emul::btor2::transform {
bool simplify_properties(hir::Module& module);
}
