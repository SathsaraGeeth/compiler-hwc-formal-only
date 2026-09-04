/*
 * compiler/btor2/include/Transform/IR/PeepholeOptimization.h
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
 * Optimizes small local BTOR2 instruction patterns
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool optimize_peepholes(Module& module);
}
