/*
 * compiler/btor2/include/Transform/IR/PropertyPartition.h
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
 * Selects BTOR2 properties by their verification category
 */

#pragma once
#include "IR/Module.h"

namespace emul::btor2::transform {
enum class PropertyPartition { safety, liveness };

bool has_properties(const Module& module, PropertyPartition partition);
Module select_properties(const Module& module, PropertyPartition partition);
}
