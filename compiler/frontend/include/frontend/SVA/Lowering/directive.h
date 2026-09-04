/*
 * compiler/frontend/include/frontend/SVA/Lowering/directive.h
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
#include "frontend/SVA/Model/directive.h"
#include "frontend/semantic_tree.h"
#include <string_view>
#include <vector>

namespace emul::frontend::sva {
Directive lower_named_directive(
    frontend::SemanticNode root,
    std::string_view top,
    std::string_view name);
std::vector<Directive> lower_directives(
    frontend::SemanticNode body);
std::vector<Directive> lower_top_directives(
    frontend::SemanticNode root,
    std::string_view top);
std::vector<Directive> lower_top_environment_directives(
    frontend::SemanticNode root,
    std::string_view top);
}
