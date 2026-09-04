/*
 * compiler/frontend/include/frontend/SVA/Lowering/expression.h
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
#include "frontend/SVA/Model/expression.h"
#include "frontend/semantic_tree.h"

namespace emul::frontend::sva {
Expression lower_expression(frontend::SemanticNode node);
}
