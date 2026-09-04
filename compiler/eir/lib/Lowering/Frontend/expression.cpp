/*
 * compiler/eir/lib/Lowering/Frontend/expression.cpp
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

#include "context.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace emul::lowering::semantic {
namespace {
std::string constant(frontend::SemanticNode node, const std::string& type) {
    auto value = node.text("constant");
    if (value.empty()) value = node.text("value");
    if (value.empty()) throw std::runtime_error("constant has no value");
    auto quote = value.find('\'');
    try {
        uint64_t number;
        if (quote == std::string::npos) number = std::stoull(value, nullptr, 10);
        else {
            auto signed_marker = value[quote + 1] == 's';
            auto base_char = static_cast<char>(
                std::tolower(value[quote + (signed_marker ? 2 : 1)]));
            auto digits = value.substr(quote + (signed_marker ? 3 : 2));
            digits.erase(std::remove(digits.begin(), digits.end(), '_'), digits.end());
            if (digits.find_first_of("xXzZ") != std::string::npos)
                return type + "'" + (digits.find_first_of("zZ") != std::string::npos ? "z" : "x");
            unsigned base = base_char == 'h' ? 16
                          : base_char == 'b' ? 2
                          : base_char == 'o' ? 8
                                             : 10;
            number = std::stoull(digits, nullptr, base);
        }
        std::ostringstream output;
        output << type << "'0x" << std::hex << number;
        return output.str();
    } catch (...) {
        throw std::runtime_error("unsupported constant: " + value);
    }
}

uint64_t lowered_constant(std::string_view value) {
    auto marker = value.find("'0x");
    if (marker == value.npos)
        throw std::runtime_error("range bound is not constant");
    return std::stoull(std::string(value.substr(marker + 3)), nullptr, 16);
}
}

LoweredValue Context::expression(frontend::SemanticNode node) {
    auto kind = node.kind();
    auto type = lower_type(node);
    if (kind == "LValueReference") {
        if (!lvalue_reference)
            throw std::runtime_error("lvalue reference outside compound assignment");
        return *lvalue_reference;
    }
    if (!node.text("constant").empty())
        return {constant(node, type), type};
    if (kind == "NamedValue" || kind == "HierarchicalValue") {
        auto name = symbol_name(node);
        if (kind == "HierarchicalValue")
            types.try_emplace(name, type);
        auto memory = memories.find(name);
        if (memory != memories.end()) {
            auto element_type = types.at(name);
            LoweredValue packed;
            for (size_t index = memory->second; index-- > 0;) {
                auto item_name = name + "[" + std::to_string(index) + "]";
                auto pending_item = pending.find(item_name);
                auto item = pending_item == pending.end()
                    ? read(item_name, element_type) : pending_item->second;
                packed = packed.name.empty() ? item : emit(
                    "concat", packed.name + ", " + item.name,
                    std::string(element_type.starts_with("4s<") ? "4s<" : "2s<") +
                        std::to_string(type_width(packed.type) +
                                       type_width(item.type)) + ">");
            }
            return packed;
        }
        return read(name, type);
    }
    if (kind == "MemberAccess") {
        auto aggregate = node.child("value");
        auto value = expression(aggregate);
        auto member = symbol_name(node);
        if (member.empty()) {
            member = node.text("member");
            auto space = member.rfind(' ');
            if (space != member.npos) member = member.substr(space + 1);
        }
        return emit("slice", value.name + ", " +
                     std::to_string(member_offset(aggregate.type(), member)) + ", " +
                     std::to_string(type_width(type)), type);
    }
    if (kind == "Call") {
        auto subroutine = find_subroutine(node.text("subroutine"));
        if (!subroutine)
            throw std::runtime_error("unknown synthesizable function: " + node.text("subroutine"));
        auto arguments = node.children("arguments");
        std::vector<frontend::SemanticNode> formals;
        for (auto member : subroutine.children("members"))
            if (member.kind() == "FormalArgument") formals.push_back(member);
        if (arguments.size() != formals.size())
            throw std::runtime_error("function argument count mismatch: " + subroutine.name());
        auto saved_values = values;
        auto saved_types = types;
        auto saved_return = return_value;
        for (size_t index = 0; index < arguments.size(); ++index) {
            types[formals[index].name()] = lower_type(formals[index]);
            values[formals[index].name()] = expression(arguments[index]);
        }
        return_value.reset();
        statement(subroutine.child("body"), nullptr, false);
        if (!return_value)
            throw std::runtime_error("synthesizable function did not return: " + subroutine.name());
        auto result = *return_value;
        values = std::move(saved_values);
        types = std::move(saved_types);
        return_value = std::move(saved_return);
        return result;
    }
    if (kind == "Inside") {
        auto subject = expression(node.child("left"));
        LoweredValue result{"4s<1>'0x0", "4s<1>"};
        for (auto item : node.children("rangeList")) {
            LoweredValue matched;
            if (item.kind() == "ValueRange") {
                auto low = expression(item.child("left"));
                auto high = expression(item.child("right"));
                auto below_low = emit("slt", subject.name + ", " + low.name, "4s<1>");
                auto high_below = emit("slt", high.name + ", " + subject.name, "4s<1>");
                auto outside = emit("or", below_low.name + ", " + high_below.name, "4s<1>");
                matched = emit("not", outside.name, "4s<1>");
            } else {
                auto candidate = expression(item);
                matched = emit("eq", subject.name + ", " + candidate.name, "4s<1>");
            }
            result = emit("or", result.name + ", " + matched.name, "4s<1>");
        }
        return result;
    }
    if (kind == "IntegerLiteral" || kind == "UnbasedUnsizedIntegerLiteral")
        return {constant(node, type), type};
    if (kind == "Conversion") {
        auto operand = expression(node.child("operand"));
        auto from = type_width(operand.type), to = type_width(type);
        if (from == to) return {operand.name, type};
        return emit(to < from ? "trunc" : "zext", operand.name, type);
    }
    if (kind == "UnaryOp") {
        auto operand = expression(node.child("operand"));
        auto op = node.text("op");
        if (op == "LogicalNot" || op == "BitwiseNot") return emit("not", operand.name, type);
        throw std::runtime_error("unsupported unary expression: " + op);
    }
    if (kind == "BinaryOp") {
        static const std::unordered_map<std::string, std::string> operations{
            {"BinaryAnd", "and"}, {"LogicalAnd", "and"}, {"BinaryOr", "or"},
            {"LogicalOr", "or"}, {"BinaryXor", "xor"}, {"Add", "add"},
            {"Subtract", "sub"}, {"Multiply", "mul"}, {"LogicalShiftLeft", "shl"},
            {"LogicalShiftRight", "lshr"}, {"ArithmeticShiftRight", "ashr"},
            {"Equality", "eq"}, {"Inequality", "ne"},
            {"CaseEquality", "eq"}, {"CaseInequality", "ne"},
            {"LessThan", "slt"},
            {"LessThanEqual", "sle"}};
        auto operation = node.text("op");
        auto left = expression(node.child("left"));
        auto right = expression(node.child("right"));
        if (operation == "GreaterThan")
            return emit("slt", right.name + ", " + left.name, type);
        if (operation == "GreaterThanEqual")
            return emit("sle", right.name + ", " + left.name, type);
        if (operation == "Mod") {
            auto divisor = lowered_constant(right.name);
            if (divisor == 0 || (divisor & (divisor - 1)) != 0)
                throw std::runtime_error("only power-of-two modulo is supported in EIR");
            auto mask = left.type + "'0x" + std::to_string(divisor - 1);
            return emit("and", left.name + ", " + mask, type);
        }
        if (operation == "Divide") {
            auto divisor = lowered_constant(right.name);
            if (divisor == 0 || (divisor & (divisor - 1)) != 0)
                throw std::runtime_error(
                    "only power-of-two division is supported in EIR");
            uint64_t shift = 0;
            while ((uint64_t{1} << shift) != divisor)
                ++shift;
            auto amount = right.type + "'0x" + std::to_string(shift);
            return emit("lshr", left.name + ", " + amount, type);
        }
        auto found = operations.find(operation);
        if (found == operations.end())
            throw std::runtime_error("unsupported binary expression: " + operation);
        return emit(found->second, left.name + ", " + right.name, type);
    }
    if (kind == "ConditionalOp") {
        auto conditions = node.children("conditions");
        if (conditions.size() != 1) throw std::runtime_error("unsupported conditional expression");
        auto select = expression(conditions[0].child("expr"));
        auto when_true = expression(node.child("left"));
        auto when_false = expression(node.child("right"));
        return emit("mux", select.name + ", " + when_false.name + ", " + when_true.name, type);
    }
    if (kind == "Concatenation") {
        auto operands = node.children("operands");
        if (operands.empty()) throw std::runtime_error("empty concatenation");
        auto result = expression(operands.front());
        for (size_t index = 1; index < operands.size(); ++index) {
            auto low = expression(operands[index]);
            auto width = type_width(result.type) + type_width(low.type);
            result = emit(
                "concat", result.name + ", " + low.name,
                "4s<" + std::to_string(width) + ">");
        }
        return result;
    }
    if (kind == "ElementSelect") {
        auto memory = symbol_name(node.child("value"));
        auto found = memories.find(memory);
        auto selector = expression(node.child("selector"));
        if (found == memories.end()) {
            auto value = expression(node.child("value"));
            auto marker = selector.name.find("'0x");
            if (marker != std::string::npos) {
                auto offset = std::stoul(
                    selector.name.substr(marker + 3), nullptr, 16);
                return emit(
                    "slice",
                    value.name + ", " + std::to_string(offset) + ", " +
                        std::to_string(type_width(type)),
                    type);
            }
            auto selector_width = type_width(selector.type);
            auto value_width = type_width(value.type);
            if (selector_width < value_width)
                selector = emit("zext", selector.name, value.type);
            auto shifted = emit(
                "lshr", value.name + ", " + selector.name, value.type);
            return emit(
                "slice", shifted.name + ", 0, " +
                    std::to_string(type_width(type)), type);
        }
        auto constant_marker = selector.name.find("'0x");
        if (constant_marker != std::string::npos) {
            auto index = std::stoull(
                selector.name.substr(constant_marker + 3), nullptr, 16);
            if (index >= found->second)
                throw std::runtime_error("memory index is out of bounds");
            return read(memory + "[" + std::to_string(index) + "]", type);
        }
        LoweredValue result;
        for (size_t index = 0; index < found->second; ++index) {
            auto element = read(memory + "[" + std::to_string(index) + "]", type);
            if (index == 0) { result = element; continue; }
            std::ostringstream literal;
            literal << selector.type << "'0x" << std::hex << index;
            auto selected = emit("eq", selector.name + ", " + literal.str(), "4s<1>");
            result = emit("mux", selected.name + ", " + result.name + ", " + element.name, type);
        }
        return result;
    }
    if (kind == "RangeSelect") {
        auto value = expression(node.child("value"));
        auto left = expression(node.child("left"));
        auto right = expression(node.child("right"));
        auto selection = node.text("selectionKind");
        if (selection == "IndexedUp" || selection == "IndexedDown") {
            auto offset = left;
            if (selection == "IndexedDown") {
                auto adjustment = left.type + "'0x" +
                    std::to_string(type_width(type) - 1);
                offset = emit("sub", left.name + ", " + adjustment, left.type);
            }
            if (type_width(offset.type) < type_width(value.type))
                offset = emit("zext", offset.name, value.type);
            else if (type_width(offset.type) > type_width(value.type))
                offset = emit("trunc", offset.name, value.type);
            auto shifted = emit(
                "lshr", value.name + ", " + offset.name, value.type);
            return emit(
                "slice", shifted.name + ", 0, " +
                    std::to_string(type_width(type)), type);
        }
        auto left_index = lowered_constant(left.name);
        auto right_index = lowered_constant(right.name);
        auto offset = std::min(left_index, right_index);
        return emit(
            "slice",
            value.name + ", " + std::to_string(offset) + ", " +
                std::to_string(type_width(type)),
            type);
    }
    throw std::runtime_error("unsupported semantic expression: " + kind);
}
}
