/*
 * compiler/btor2/lib/Lowering/Formal/lower_expression.cpp
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

#include "Lowering/Formal/lower_expression.h"
#include "Lowering/Formal/history.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace emul::formal {
namespace {
std::string bits(std::string text, uint32_t width) {
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    auto quote = text.find('\'');
    auto marker = quote == text.npos ? 'd' :
        static_cast<char>(std::tolower(text[quote + 1]));
    auto digits = quote == text.npos ? text : text.substr(quote + 2);
    if (digits.find_first_of("xXzZ?") != std::string::npos)
        throw std::runtime_error(
            "X/Z literals are not valid in two-state formal expressions");
    auto base = marker == 'h' ? 16 : marker == 'o' ? 8 :
                marker == 'b' ? 2 : 10;
    uint64_t value = 0;
    try {
        value = std::stoull(digits, nullptr, base);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid formal integer literal: " + text);
    }
    std::string result(width, '0');
    for (uint32_t index = 0; index < width && index < 64; ++index)
        if ((value >> index) & 1)
            result[width - index - 1] = '1';
    return result;
}

Btor2Value unary(
    const frontend::sva::Expression& expression,
    TransitionSystem& system,
    Btor2Value sample) {
    auto value = lower_sva_expression(
        expression.operands.at(0), system, sample);
    auto operation = expression.operation;
    if (operation == "LogicalNot")
        return system.builder.unary(
            "not", as_boolean(value, system), 1);
    if (operation == "BitwiseNot")
        return system.builder.unary("not", value);
    if (operation == "ReductionAnd")
        return system.builder.unary("redand", value, 1);
    if (operation == "ReductionOr")
        return system.builder.unary("redor", value, 1);
    if (operation == "ReductionXor")
        return system.builder.unary("redxor", value, 1);
    throw std::runtime_error("unsupported formal unary expression: " +
                             operation);
}

Btor2Value binary(
    const frontend::sva::Expression& expression,
    TransitionSystem& system,
    Btor2Value sample) {
    auto left = lower_sva_expression(
        expression.operands.at(0), system, sample);
    auto right = lower_sva_expression(
        expression.operands.at(1), system, sample);
    auto operation = expression.operation;
    if (operation == "LogicalAnd" || operation == "LogicalOr" ||
        operation == "LogicalImplication") {
        left = as_boolean(left, system);
        right = as_boolean(right, system);
    }
    if (operation == "LogicalImplication") {
        auto not_left = system.builder.unary("not", left, 1);
        return system.builder.binary("or", not_left, right, 1);
    }
    static const std::unordered_map<std::string, std::string> operations = {
        {"Add", "add"}, {"Subtract", "sub"}, {"Multiply", "mul"},
        {"Divide", "udiv"}, {"Mod", "urem"},
        {"BitwiseAnd", "and"}, {"BitwiseOr", "or"},
        {"BitwiseXor", "xor"}, {"LogicalAnd", "and"},
        {"LogicalOr", "or"}, {"Equality", "eq"},
        {"Inequality", "neq"}, {"CaseEquality", "eq"},
        {"CaseInequality", "neq"}, {"LessThan", "ult"},
        {"LessThanEqual", "ulte"}, {"GreaterThan", "ugt"},
        {"GreaterThanEqual", "ugte"}, {"LogicalShiftLeft", "sll"},
        {"LogicalShiftRight", "srl"}, {"ArithmeticShiftLeft", "sll"},
        {"ArithmeticShiftRight", "sra"}
    };
    auto found = operations.find(operation);
    if (found == operations.end())
        throw std::runtime_error("unsupported formal binary expression: " +
                                 operation);
    auto comparison = operation.find("Equal") != std::string::npos ||
        operation == "Equality" || operation == "Inequality" ||
        operation == "LessThan" || operation == "GreaterThan";
    return system.builder.binary(
        found->second, left, right, comparison ? 1 : expression.width);
}

uint32_t constant_count(const frontend::sva::Expression& expression) {
    if (expression.kind != frontend::sva::ExpressionKind::constant)
        throw std::runtime_error("$past depth must be a constant expression");
    auto text = expression.value;
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    auto quote = text.find('\'');
    auto marker = quote == text.npos ? 'd' :
        static_cast<char>(std::tolower(text[quote + 1]));
    auto digits = quote == text.npos ? text : text.substr(quote + 2);
    auto base = marker == 'h' ? 16 : marker == 'o' ? 8 :
                marker == 'b' ? 2 : 10;
    try {
        return static_cast<uint32_t>(std::stoul(digits, nullptr, base));
    } catch (const std::exception&) {
        throw std::runtime_error("invalid $past depth: " + expression.value);
    }
}

Btor2Value call(
    const frontend::sva::Expression& expression,
    TransitionSystem& system,
    Btor2Value sample) {
    if (expression.operands.empty())
        throw std::runtime_error("sampled function requires an argument");
    auto value = lower_sva_expression(
        expression.operands.front(), system, sample);
    auto name = expression.operation;
    if (name == "$sampled")
        return value;
    if (name == "$past") {
        if (expression.operands.size() > 2)
            throw std::runtime_error(
                "$past default value and explicit clock are not supported");
        auto ticks = expression.operands.size() == 2 ?
            constant_count(expression.operands[1]) : 1;
        return SampledHistory::previous(value, ticks, system, sample);
    }
    if (expression.operands.size() != 1)
        throw std::runtime_error(name + " accepts exactly one argument");
    auto history = SampledHistory::previous(value, 1, system, sample);
    auto current_boolean = as_boolean(value, system);
    auto previous_boolean = as_boolean(history, system);
    if (name == "$rose")
        return system.builder.binary(
            "and", current_boolean,
            system.builder.unary("not", previous_boolean, 1), 1);
    if (name == "$fell")
        return system.builder.binary(
            "and", previous_boolean,
            system.builder.unary("not", current_boolean, 1), 1);
    auto equal = system.builder.binary("eq", value, history, 1);
    if (name == "$stable")
        return equal;
    if (name == "$changed")
        return system.builder.unary("not", equal, 1);
    throw std::runtime_error("unsupported sampled system function: " + name);
}
}

Btor2Value as_boolean(Btor2Value value, TransitionSystem& system) {
    if (value.width == 1)
        return value;
    auto zero = system.builder.constant(value.width,
                                        std::string(value.width, '0'));
    return system.builder.binary("neq", value, zero, 1);
}

Btor2Value lower_sva_expression(
    const frontend::sva::Expression& expression,
    TransitionSystem& system,
    Btor2Value sample) {
    using frontend::sva::ExpressionKind;
    if (expression.kind == ExpressionKind::signal) {
        auto found = system.signals.find(expression.value);
        if (found == system.signals.end())
            throw std::runtime_error("formal signal not found: " +
                                     expression.value);
        auto value = found->second;
        if (value.width < expression.width)
            return system.builder.extend("uext", value, expression.width);
        if (value.width > expression.width)
            return system.builder.slice(value, 0, expression.width);
        return value;
    }
    if (expression.kind == ExpressionKind::constant)
        return system.builder.constant(
            expression.width, bits(expression.value, expression.width));
    if (expression.kind == ExpressionKind::unary)
        return unary(expression, system, sample);
    if (expression.kind == ExpressionKind::binary)
        return binary(expression, system, sample);
    if (expression.kind == ExpressionKind::conditional) {
        auto condition = as_boolean(
            lower_sva_expression(expression.operands.at(0), system, sample), system);
        return system.builder.ternary(
            "ite", condition,
            lower_sva_expression(expression.operands.at(1), system, sample),
            lower_sva_expression(expression.operands.at(2), system, sample));
    }
    if (expression.kind == ExpressionKind::concatenation) {
        if (expression.operands.empty())
            throw std::runtime_error("empty formal concatenation");
        auto result = lower_sva_expression(
            expression.operands.front(), system, sample);
        for (size_t index = 1; index < expression.operands.size(); ++index) {
            auto low = lower_sva_expression(
                expression.operands[index], system, sample);
            result = system.builder.binary(
                "concat", result, low, result.width + low.width);
        }
        return result;
    }
    if (expression.kind == ExpressionKind::selection) {
        auto value = lower_sva_expression(expression.operands.at(0), system, sample);
        auto selector = lower_sva_expression(expression.operands.at(1), system, sample);
        if (selector.width < value.width)
            selector = system.builder.extend("uext", selector, value.width);
        else if (selector.width > value.width)
            selector = system.builder.slice(selector, 0, value.width);
        auto shifted = system.builder.binary("srl", value, selector, value.width);
        return system.builder.slice(shifted, 0, expression.width);
    }
    if (expression.kind == ExpressionKind::call)
        return call(expression, system, sample);
    throw std::runtime_error("unsupported formal expression kind");
}
}
