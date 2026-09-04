/*
 * compiler/btor2/lib/IR/Operation.cpp
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

#include "../../include/IR/Operation.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>

namespace emul::btor2 {
namespace {
template <size_t Size>
bool contains(const std::array<std::string_view, Size>& values,
              std::string_view value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

void require_width(std::uint32_t width) {
    if (!width)
        throw std::runtime_error("zero-width BTOR2 operation");
}

void require_operator(OperatorClass kind, std::string_view opcode) {
    if (!is_operator(kind, opcode))
        throw std::runtime_error(
            "BTOR2 operator is not defined by spec/btor2.yaml: " +
            std::string(opcode));
}

std::string sanitize_symbol(std::string_view name) {
    std::string result(name);
    std::replace_if(result.begin(), result.end(), [](char character) {
        return !(std::isalnum(static_cast<unsigned char>(character)) ||
                 character == '_' || character == '.');
    }, '_');
    return result;
}
}

bool is_operator(OperatorClass operator_class, std::string_view opcode) {
    static constexpr std::array<std::string_view, 4> unary = {
        "not", "redand", "redor", "redxor"
    };
    static constexpr std::array<std::string_view, 22> binary = {
        "and", "or", "xor", "add", "sub", "mul", "udiv", "urem",
        "eq", "neq", "ult", "ulte", "ugt", "ugte", "slt", "slte",
        "sgt", "sgte", "sll", "srl", "sra", "concat"
    };
    static constexpr std::array<std::string_view, 1> ternary = {"ite"};
    static constexpr std::array<std::string_view, 2> extension = {
        "uext", "sext"
    };
    switch (operator_class) {
    case OperatorClass::unary:
        return contains(unary, opcode);
    case OperatorClass::binary:
        return contains(binary, opcode);
    case OperatorClass::ternary:
        return contains(ternary, opcode);
    case OperatorClass::extension:
        return contains(extension, opcode);
    }
    return false;
}

Operation Operation::input(std::uint32_t width, std::string_view name) {
    require_width(width);
    return {"input", width, {}, {}, sanitize_symbol(name)};
}

Operation Operation::state(std::uint32_t width, std::string_view name) {
    require_width(width);
    return {"state", width, {}, {}, sanitize_symbol(name)};
}

Operation Operation::constant(std::uint32_t width, std::string_view bits) {
    require_width(width);
    if (bits.size() != width ||
        bits.find_first_not_of("01") != std::string_view::npos)
        throw std::runtime_error(
            "BTOR2 constant does not match spec/btor2.yaml");
    return {"const", width, {}, {}, {}, std::string(bits)};
}

Operation Operation::unary(
    std::string_view opcode,
    Value value,
    std::uint32_t result_width) {
    require_operator(OperatorClass::unary, opcode);
    const auto width = result_width ? result_width : value.width;
    require_width(width);
    return {std::string(opcode), width,
            {static_cast<NodeId>(value.node)}};
}

Operation Operation::binary(
    std::string_view opcode,
    Value left,
    Value right,
    std::uint32_t result_width) {
    require_operator(OperatorClass::binary, opcode);
    const auto width = result_width ? result_width : left.width;
    require_width(width);
    return {std::string(opcode), width,
            {static_cast<NodeId>(left.node),
             static_cast<NodeId>(right.node)}};
}

Operation Operation::ternary(
    std::string_view opcode,
    Value condition,
    Value left,
    Value right) {
    require_operator(OperatorClass::ternary, opcode);
    if (condition.width != 1 || left.width != right.width)
        throw std::runtime_error("invalid BTOR2 ternary operands");
    return {std::string(opcode), left.width,
            {static_cast<NodeId>(condition.node),
             static_cast<NodeId>(left.node),
             static_cast<NodeId>(right.node)}};
}

Operation Operation::slice(
    Value value,
    std::uint32_t offset,
    std::uint32_t width) {
    if (!width || offset > value.width || width > value.width - offset)
        throw std::runtime_error("invalid BTOR2 slice");
    return {"slice", width, {static_cast<NodeId>(value.node)},
            {offset + width - 1, offset}};
}

Operation Operation::extend(
    std::string_view opcode,
    Value value,
    std::uint32_t width) {
    require_operator(OperatorClass::extension, opcode);
    if (width < value.width)
        throw std::runtime_error("BTOR2 extension narrows a value");
    return {std::string(opcode), width,
            {static_cast<NodeId>(value.node)}, {width - value.width}};
}
} 
