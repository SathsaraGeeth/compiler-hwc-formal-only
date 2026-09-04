/*
 * compiler/btor2/include/Lowering/Hierarchy.h
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
 * Flattens an EIR program hierarchy for BTOR2 lowering
 */

#pragma once

#include "eir/include/IR/Module.h"

namespace emul::btor2 {
eir::Module flatten_hierarchy(const eir::Program& program);
}
