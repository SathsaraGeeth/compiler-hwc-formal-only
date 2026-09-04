/*
 * compiler/btor2/include/Transform/IR/StrengthReduction.h
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
 * Replaces expensive BTOR2 operations with simpler operations
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool reduce_strength(Module& module);
}
