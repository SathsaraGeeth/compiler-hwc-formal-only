/*
 * compiler/btor2/include/Transform/HIR/PassPipeline.h
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
 * Runs higher level BTOR2 property optimization passes
 */

#pragma once
#include "HIR/Module.h"

namespace emul::btor2::transform {
bool optimize(hir::Module& module);
}
