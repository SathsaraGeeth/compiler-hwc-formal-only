/*
 * compiler/frontend/include/frontend/SVA/Lowering/sequence.h
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
#include "frontend/SVA/Model/sequence.h"
#include "frontend/semantic_tree.h"

namespace emul::frontend::sva {
Sequence lower_sequence(frontend::SemanticNode node);
}
