/*
 * compiler/frontend/include/frontend/SVA/Model/sequence.h
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
#include <cstdint>
#include <limits>
#include <memory>

namespace emul::frontend::sva {
enum class SequenceKind {
    atom,
    delay,
    concatenation,
    consecutive_repeat,
    nonconsecutive_repeat,
    goto_repeat,
    conjunction,
    disjunction,
    intersection,
    first_match,
    match,
    throughout,
    within
};

struct Sequence {
    static constexpr uint32_t unbounded =
        std::numeric_limits<uint32_t>::max();
    SequenceKind kind = SequenceKind::atom;
    uint32_t minimum = 0;
    uint32_t maximum = 0;
    Expression expression;
    std::shared_ptr<Sequence> left;
    std::shared_ptr<Sequence> right;
    std::vector<Expression> match_items;
};
}
