/*
 * compiler/frontend/include/frontend/SVA/Lowering/property_model.h
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
#include "frontend/SVA/Model/property.h"
#include "frontend/semantic_tree.h"

namespace emul::frontend::sva {
Property lower_property(frontend::SemanticNode node);
}
