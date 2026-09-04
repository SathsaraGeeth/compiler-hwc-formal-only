/*
 * compiler/btor2/include/Conversion/HIRToIR.h
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
 * Converts higher level temporal operations into concrete BTOR2 IR
 */

#pragma once
#include "HIR/Module.h"
#include "Lowering/Formal/system.h"
#include <string_view>

namespace emul::formal {
enum class PropertyTarget;

void convert_hir_to_ir(
    const btor2::hir::Module& module,
    std::string_view selected_property,
    PropertyTarget target,
    TransitionSystem& system);
}
