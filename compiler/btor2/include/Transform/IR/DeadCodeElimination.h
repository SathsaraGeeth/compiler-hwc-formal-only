/*
 * compiler/btor2/include/Transform/IR/DeadCodeElimination.h
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
 * Removes unused BTOR2 operations
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool eliminate_dead_code(Module& module);
}
