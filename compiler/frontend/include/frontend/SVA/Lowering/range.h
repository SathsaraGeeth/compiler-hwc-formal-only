/*
 * compiler/frontend/include/frontend/SVA/Lowering/range.h
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
#include "frontend/semantic_tree.h"
#include <cstdint>
#include <utility>

namespace emul::frontend::sva {
std::pair<uint32_t, uint32_t> lower_range(
    frontend::SemanticNode node);
}
