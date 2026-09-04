/*
 * compiler/btor2/include/Lowering/Formal/harness.h
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
 * Builds the formal harness that joins hardware and property directives
 */

#pragma once

#include "eir/include/IR/Module.h"
#include "frontend/SVA/Model/directive.h"
#include "frontend/elaborated_design.h"
#include <string_view>
#include <string>
#include <utility>
#include <vector>

namespace emul::formal {
struct HostFlag {
    std::string name;
    std::string clock;
};

struct Harness {
    eir::Program hardware;
    std::vector<frontend::sva::Directive> directives;
    std::vector<std::pair<std::string, std::string>> signal_aliases;
    std::vector<std::pair<std::string, std::string>> instance_aliases;
    std::vector<HostFlag> host_flags;
};

Harness build_harness(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property);
}
