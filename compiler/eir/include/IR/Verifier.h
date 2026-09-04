/*
 * compiler/eir/include/IR/Verifier.h
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
 * 1. Check the EIR correctness
 */

#pragma once
#include "Module.h"

namespace emul::eir {
    void verify(const Program& program); 
}
