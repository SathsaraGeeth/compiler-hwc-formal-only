/*
 * compiler/btor2/include/Transform/IR/ConstantFolding.h
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
 * Evaluates constant BTOR2 expressions
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool fold_constants(Module& module);
}
