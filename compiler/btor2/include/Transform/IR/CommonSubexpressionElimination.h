/*
 * compiler/btor2/include/Transform/IR/CommonSubexpressionElimination.h
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
 * Removes repeated BTOR2 expressions
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool eliminate_common_subexpressions(Module& module);
}
