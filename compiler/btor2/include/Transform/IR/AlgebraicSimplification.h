/*
 * compiler/btor2/include/Transform/IR/AlgebraicSimplification.h
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
 * Simplifies BTOR2 expressions using algebraic rules
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool simplify_algebra(Module& module);
}
