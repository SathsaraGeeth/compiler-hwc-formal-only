/*
 * compiler/frontend/include/frontend/SVA/Model/directive.h
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
#include <string>
#include <vector>

namespace emul::frontend::sva {
enum class DirectiveKind {
    assert_property,
    assume_property,
    cover_property,
    restrict_property
};

enum class Edge {
    any,
    posedge,
    negedge
};

struct Clock {
    std::string signal;
    Edge edge = Edge::any;
};

struct LocalVariable {
    std::string name;
    uint32_t width = 1;
};

struct Directive {
    std::string name;
    DirectiveKind kind = DirectiveKind::assert_property;
    Clock clock;
    Expression disable;
    Property property;
    std::vector<LocalVariable> locals;
};

}
