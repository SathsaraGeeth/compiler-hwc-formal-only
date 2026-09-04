/*
 * compiler/btor2/include/Transform/IR/PassSupport.h
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
 * Provides shared helpers for BTOR2 IR transformation passes
 */

#pragma once
#include "IR/Module.h"
#include <vector>

namespace emul::btor2::transform {
NodeId resolve(NodeId value, const std::vector<NodeId>& replacements);
bool apply_replacements(Module& module,
                        const std::vector<NodeId>& replacements);
void retain(Module& module, const std::vector<bool>& live);
}
