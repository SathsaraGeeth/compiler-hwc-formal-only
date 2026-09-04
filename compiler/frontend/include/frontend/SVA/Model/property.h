/*
 * compiler/frontend/include/frontend/SVA/Model/property.h
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
#include <memory>
#include <vector>

namespace emul::frontend::sva {
enum class PropertyKind {
    sequence,
    negation,
    conjunction,
    disjunction,
    implication,
    iff,
    nexttime,
    always,
    eventually,
    until,
    conditional,
    case_property,
    accept_on,
    reject_on
};

struct Property {
    PropertyKind kind = PropertyKind::sequence;
    bool strong = false;
    bool overlapped = true;
    bool inclusive = false;
    bool followed_by = false;
    bool synchronous = false;
    uint32_t minimum = 0;
    uint32_t maximum = 0;
    std::shared_ptr<Sequence> sequence;
    std::shared_ptr<Property> left;
    std::shared_ptr<Property> right;
    Expression condition;
    std::vector<Expression> case_matches;
    std::vector<std::shared_ptr<Property>> alternatives;
};
}
