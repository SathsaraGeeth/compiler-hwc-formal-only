/*
 * compiler/frontend/include/frontend/SVA/Model/expression.h
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
#include <cstdint>
#include <string>
#include <vector>

namespace emul::frontend::sva {
enum class ExpressionKind {
    signal,
    constant,
    unary,
    binary,
    conditional,
    concatenation,
    selection,
    call,
    assignment
};

struct Expression {
    ExpressionKind kind = ExpressionKind::constant;
    std::string operation;
    std::string value;
    uint32_t width = 1;
    std::vector<Expression> operands;
};
}
