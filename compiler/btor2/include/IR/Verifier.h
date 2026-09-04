/*
 * compiler/btor2/include/IR/Verifier.h
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
 * 1. Checks structural correctness of an in memory BTOR2 module.
 * 2. Checks operations
 *    - every operation has a nonzero width
 *    - every operand exists and dominates its user
 * 3. Checks states
 *    - the value refers to a state declaration
 *    - initial and next expressions match the state width
 *    - every state has a next expression
 * 4. Checks properties
 *    - outputs reference valid operations
 *    - constraint, bad, cover, and fair conditions are Boolean
 *    - justice has at least one condition
 *    - every justice condition is Boolean
 * 5. Methods
 *    - verify; throws an error when the module is malformed
 */

#pragma once
#include "Module.h"
namespace emul::btor2 {
void verify(const Module& module);
} // namespace emul::btor2
