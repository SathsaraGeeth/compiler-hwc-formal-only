/*
 * compiler/btor2/include/Transform/IR/InstructionCombining.h
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
 * Combines compatible BTOR2 operations
 */

#pragma once
#include "IR/Module.h"
namespace emul::btor2::transform {
bool combine_instructions(Module& module);
}
