/*
 * compiler/btor2/include/Transform/IR/CopyPropagation.h
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
 * Replaces copied BTOR2 values with their original values
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool propagate_copies(Module& module);
}
