/*
 * compiler/eir/lib/Lowering/Frontend/module.h
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
 */

#pragma once
#include "../../../include/IR/Module.h"
#include "frontend/elaborated_design.h"

namespace emul::lowering::semantic {
eir::Program lower_design(const frontend::ElaboratedDesign& design);
eir::Module lower_module(frontend::SemanticNode body);
}
