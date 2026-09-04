/*
 * compiler/btor2/include/Transform/HIR/ComponentDeduplication.h
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
 * Removes duplicate higher level property components
 */

#pragma once
#include "HIR/Module.h"

namespace emul::btor2::transform {
bool deduplicate_components(hir::Module& module);
}
