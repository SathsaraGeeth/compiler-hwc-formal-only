/*
 * compiler/frontend/lib/SVA/Lowering/expression.cpp
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

#include "frontend/SVA/Lowering/expression.h"
#include "eir/lib/Lowering/Frontend/context.h"
#include <stdexcept>
#include <utility>

namespace emul::frontend::sva {
namespace {
using frontend::SemanticNode;

std::string callable(SemanticNode node) {
    auto name = node.text("subroutine");
    auto space = name.find(' ');
    return space == name.npos ? name : name.substr(space + 1);
}

uint32_t width(SemanticNode node) {
    auto type = lowering::semantic::lower_type(node);
    return lowering::semantic::type_width(type);
}

Expression unary(SemanticNode node) {
    return {
        ExpressionKind::unary,
        node.text("op"),
        {},
        width(node),
        {lower_expression(node.child("operand"))}
    };
}

Expression binary(SemanticNode node) {
    return {
        ExpressionKind::binary,
        node.text("op"),
        {},
        width(node),
        {
            lower_expression(node.child("left")),
            lower_expression(node.child("right"))
        }
    };
}

Expression condition(SemanticNode node) {
    auto conditions = node.children("conditions");
    if (conditions.empty())
        return lower_expression(node.child("condition"));
    auto result = lower_expression(conditions.front().child("expr"));
    for (size_t index = 1; index < conditions.size(); ++index) {
        result = {
            ExpressionKind::binary,
            "LogicalAnd",
            {},
            1,
            {std::move(result),
             lower_expression(conditions[index].child("expr"))}
        };
    }
    return result;
}
}

Expression lower_expression(SemanticNode node) {
    if (!node)
        return {};
    if (node.kind() == "Conversion") {
        auto result = lower_expression(node.child("operand"));
        result.width = width(node);
        return result;
    }
    if (node.kind() == "NamedValue" ||
        node.kind() == "HierarchicalValue") {
        if (!node.text("constant").empty())
            return {
                ExpressionKind::constant,
                {},
                node.text("constant"),
                width(node),
                {}
            };
        return {
            ExpressionKind::signal,
            {},
            lowering::semantic::symbol_name(node),
            width(node),
            {}
        };
    }
    if (node.kind() == "IntegerLiteral" ||
        node.kind() == "UnbasedUnsizedIntegerLiteral") {
        auto value = node.text("constant");
        if (value.empty())
            value = node.text("value");
        return {ExpressionKind::constant, {}, value, width(node), {}};
    }
    if (node.kind() == "UnaryOp")
        return unary(node);
    if (node.kind() == "BinaryOp")
        return binary(node);
    if (node.kind() == "ConditionalOp") {
        return {
            ExpressionKind::conditional,
            {},
            {},
            width(node),
            {
                condition(node),
                lower_expression(node.child("left")),
                lower_expression(node.child("right"))
            }
        };
    }
    if (node.kind() == "Concatenation") {
        Expression result{
            ExpressionKind::concatenation, {}, {}, width(node), {}};
        for (auto operand : node.children("operands"))
            result.operands.push_back(lower_expression(operand));
        return result;
    }
    if (node.kind() == "ElementSelect") {
        return {
            ExpressionKind::selection,
            "ElementSelect",
            {},
            width(node),
            {
                lower_expression(node.child("value")),
                lower_expression(node.child("selector"))
            }
        };
    }
    if (node.kind() == "Call") {
        Expression result{
            ExpressionKind::call,
            callable(node),
            {},
            width(node),
            {}
        };
        for (auto argument : node.children("arguments"))
            result.operands.push_back(lower_expression(argument));
        return result;
    }
    if (node.kind() == "Assignment") {
        return {
            ExpressionKind::assignment,
            node.boolean("isNonBlocking") ? "nonblocking" : "blocking",
            {},
            width(node),
            {
                lower_expression(node.child("left")),
                lower_expression(node.child("right"))
            }
        };
    }
    throw std::runtime_error(
        "unsupported SVA value expression: " + node.kind());
}
}
