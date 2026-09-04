/*
 * compiler/eir/include/MIR/Verifier.h
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.1
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 * 1.1     | Verify mutable virtual-register assignments
 */

/*
 * Comments:
 * 1. Check MIR correctness.
 * 2. Checks types, symbols, register assignment, operand kinds, signatures,
 *    block structure, and yield results.
 * 3. Returns normally on success and throws a diagnostic on failure.
 */

#pragma once
#include "MachineModule.h"

namespace emul::mir {
    void verify(const MachineModule& module);
}
