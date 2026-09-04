/*
 * compiler/btor2/include/Transform/IR/PassPipeline.h
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
 * Runs BTOR2 IR optimization passes
 */

#pragma once
#include "IR/Module.h"

namespace emul::btor2::transform {
bool optimize(Module& module);
}
