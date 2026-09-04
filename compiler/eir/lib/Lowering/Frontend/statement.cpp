/*
 * compiler/eir/lib/Lowering/Frontend/statement.cpp
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
#include <sstream>
#include <stdexcept>

namespace emul::lowering::semantic {
namespace {
LoweredValue conjunction(Context& context, const LoweredValue* parent,
                         const LoweredValue& condition) {
    return parent ? context.emit("and", parent->name + ", " + condition.name, "4s<1>")
                  : condition;
}

LoweredValue convert(
    Context& context,
    LoweredValue value,
    const std::string& type) {
    auto source_width = type_width(value.type);
    auto target_width = type_width(type);
    if (source_width == target_width)
        return {value.name, type};
    return context.emit(
        source_width < target_width ? "zext" : "trunc",
        value.name, type);
}

uint64_t literal_number(std::string_view value) {
    auto marker = value.find("'0x");
    if (marker == value.npos)
        throw std::runtime_error("compile-time loop value is not constant");
    return std::stoull(std::string(value.substr(marker + 3)), nullptr, 16);
}

std::string literal(const std::string& type, uint64_t value) {
    std::ostringstream output;
    output << type << "'0x" << std::hex << value;
    return output.str();
}

frontend::SemanticNode strip_conversions(frontend::SemanticNode node) {
    while (node && node.kind() == "Conversion")
        node = node.child("operand");
    return node;
}
}

void Context::assign(frontend::SemanticNode target, LoweredValue value,
                     const LoweredValue* enable, bool sequential) {
    if (target.kind() == "Concatenation") {
        auto operands = target.children("operands");
        unsigned offset = 0;
        for (auto item = operands.rbegin(); item != operands.rend(); ++item) {
            auto item_type = lower_type(*item);
            auto width = type_width(item_type);
            auto slice = emit("slice", value.name + ", " + std::to_string(offset) + ", " +
                                      std::to_string(width), item_type);
            assign(*item, slice, enable, sequential);
            offset += width;
        }
        return;
    }
    if (target.kind() == "MemberAccess") {
        auto aggregate = target.child("value");
        auto old = expression(aggregate);
        auto member = target.text("member");
        auto space = member.rfind(' ');
        if (space != member.npos) member = member.substr(space + 1);
        auto offset = member_offset(aggregate.type(), member);
        auto width = type_width(lower_type(target));
        value = convert(*this, std::move(value), lower_type(target));
        LoweredValue replacement = value;
        if (offset) {
            auto low = emit("slice", old.name + ", 0, " + std::to_string(offset),
                            "4s<" + std::to_string(offset) + ">");
            replacement = emit("concat", replacement.name + ", " + low.name,
                               "4s<" + std::to_string(width + offset) + ">");
        }
        auto high_offset = offset + width;
        auto aggregate_width = type_width(old.type);
        if (high_offset < aggregate_width) {
            auto high_width = aggregate_width - high_offset;
            auto high = emit("slice", old.name + ", " + std::to_string(high_offset) +
                             ", " + std::to_string(high_width),
                             "4s<" + std::to_string(high_width) + ">");
            replacement = emit("concat", high.name + ", " + replacement.name, old.type);
        }
        assign(aggregate, replacement, enable, sequential);
        return;
    }
    if (target.kind() == "RangeSelect") {
        auto aggregate = target.child("value");
        auto old = expression(aggregate);
        auto left = expression(target.child("left"));
        auto selection = target.text("selectionKind");
        LoweredValue offset;
        if (selection == "IndexedUp" || selection == "IndexedDown") {
            offset = left;
            if (selection == "IndexedDown") {
                auto adjustment = left.type + "'0x" +
                    std::to_string(type_width(lower_type(target)) - 1);
                offset = emit("sub", left.name + ", " + adjustment, left.type);
            }
        } else {
            auto right = expression(target.child("right"));
            auto left_index = literal_number(left.name);
            auto right_index = literal_number(right.name);
            auto index = std::min(left_index, right_index);
            offset = {literal(old.type, index), old.type};
        }
        if (type_width(offset.type) < type_width(old.type))
            offset = emit("zext", offset.name, old.type);
        else if (type_width(offset.type) > type_width(old.type))
            offset = emit("trunc", offset.name, old.type);

        auto selected_type = lower_type(target);
        value = convert(*this, std::move(value), selected_type);
        auto wide_value = type_width(selected_type) == type_width(old.type)
            ? LoweredValue{value.name, old.type}
            : emit("zext", value.name, old.type);
        auto selected_ones = emit(
            "not", selected_type + "'0x0", selected_type);
        auto wide_mask = type_width(selected_type) == type_width(old.type)
            ? LoweredValue{selected_ones.name, old.type}
            : emit("zext", selected_ones.name, old.type);
        auto shifted_value = emit(
            "shl", wide_value.name + ", " + offset.name, old.type);
        auto shifted_mask = emit(
            "shl", wide_mask.name + ", " + offset.name, old.type);
        auto inverse_mask = emit("not", shifted_mask.name, old.type);
        auto preserved = emit(
            "and", old.name + ", " + inverse_mask.name, old.type);
        auto replacement = emit(
            "or", preserved.name + ", " + shifted_value.name, old.type);
        assign(aggregate, replacement, enable, sequential);
        return;
    }
    if (target.kind() == "ElementSelect") {
        auto memory = symbol_name(target.child("value"));
        auto found = memories.find(memory);
        if (found == memories.end()) {
            auto aggregate = target.child("value");
            auto old = expression(aggregate);
            auto selector = expression(target.child("selector"));
            auto marker = selector.name.find("'0x");
            if (marker == std::string::npos)
                throw std::runtime_error("dynamic packed indexed assignment is not yet supported");
            auto offset = std::stoull(selector.name.substr(marker + 3), nullptr, 16);
            auto width = type_width(lower_type(target));
            value = convert(*this, std::move(value), lower_type(target));
            LoweredValue replacement = value;
            if (offset) {
                auto low = emit("slice", old.name + ", 0, " + std::to_string(offset),
                                "4s<" + std::to_string(offset) + ">");
                replacement = emit("concat", replacement.name + ", " + low.name,
                                   "4s<" + std::to_string(width + offset) + ">");
            }
            auto high_offset = offset + width;
            auto aggregate_width = type_width(old.type);
            if (high_offset < aggregate_width) {
                auto high_width = aggregate_width - high_offset;
                auto high = emit("slice", old.name + ", " + std::to_string(high_offset) +
                                 ", " + std::to_string(high_width),
                                 "4s<" + std::to_string(high_width) + ">");
                replacement = emit("concat", high.name + ", " + replacement.name, old.type);
            }
            assign(aggregate, replacement, enable, sequential);
            return;
        }
        value = convert(*this, std::move(value), types.at(memory));
        auto selector = expression(target.child("selector"));
        auto& assignments = sequential ? pending : values;
        auto constant_marker = selector.name.find("'0x");
        if (constant_marker != std::string::npos) {
            auto index = std::stoull(
                selector.name.substr(constant_marker + 3), nullptr, 16);
            if (index >= found->second)
                throw std::runtime_error("memory index is out of bounds");
            auto name = memory + "[" + std::to_string(index) + "]";
            auto old = assignments.contains(name) ? assignments.at(name)
                                                  : read(name, value.type);
            assignments[name] = enable
                ? emit("mux", enable->name + ", " + old.name + ", " +
                              value.name, value.type)
                : value;
            return;
        }
        for (size_t index = 0; index < found->second; ++index) {
            auto name = memory + "[" + std::to_string(index) + "]";
            auto old = assignments.contains(name) ? assignments.at(name) : read(name, value.type);
            auto equal = emit("eq", selector.name + ", " +
                                      literal(selector.type, index), "4s<1>");
            auto active = conjunction(*this, enable, equal);
            assignments[name] = emit("mux", active.name + ", " + old.name + ", " + value.name,
                                     value.type);
        }
        return;
    }
    if (target.kind() != "NamedValue" && target.kind() != "HierarchicalValue")
        throw std::runtime_error("unsupported assignment target: " + target.kind());
    auto name = symbol_name(target);
    if (target.kind() == "HierarchicalValue")
        types.try_emplace(name, lower_type(target));
    auto memory = memories.find(name);
    if (memory != memories.end()) {
        auto element_type = types.at(name);
        auto element_width = type_width(element_type);
        for (size_t index = 0; index < memory->second; ++index) {
            auto item = type_width(value.type) == element_width
                ? LoweredValue{value.name, element_type}
                : emit("slice", value.name + ", " +
                       std::to_string(index * element_width) + ", " +
                       std::to_string(element_width), element_type);
            auto item_name = name + "[" + std::to_string(index) + "]";
            auto& assignments = sequential ? pending : values;
            auto old = assignments.contains(item_name) ? assignments.at(item_name)
                                                       : read(item_name, element_type);
            assignments[item_name] = enable
                ? emit("mux", enable->name + ", " + old.name + ", " + item.name,
                       element_type)
                : item;
        }
        return;
    }
    value = convert(*this, std::move(value), types.at(name));
    if (sequential) {
        auto old = pending.contains(name) ? pending.at(name) : read(name, value.type);
        pending[name] = enable
            ? emit("mux", enable->name + ", " + old.name + ", " + value.name, value.type)
            : value;
    } else {
        if (!enable) values[name] = value;
        else {
            auto old = values.contains(name) ? values.at(name) : read(name, value.type);
            values[name] = emit("mux", enable->name + ", " + old.name + ", " + value.name,
                                value.type);
        }
    }
}

void Context::statement(frontend::SemanticNode node, const LoweredValue* enable,
                        bool sequential) {
    auto kind = node.kind();
    if (kind == "Block") return statement(node.child("body"), enable, sequential);
    if (kind == "List") {
        for (auto item : node.children("list")) statement(item, enable, sequential);
        return;
    }
    if (kind == "VariableDeclaration") {
        auto variable = node.child("symbol");
        auto name = variable.name();
        types[name] = lower_type(variable);
        auto initializer = variable.child("initializer");
        if (initializer) values[name] = expression(initializer);
        else values[name] = {types[name] + "'0x0", types[name]};
        return;
    }
    if (kind == "Return") {
        return_value = expression(node.child("expr"));
        return;
    }
    if (kind == "ForLoop") {
        if (!node.children("initializers").empty())
            throw std::runtime_error(
                "hardware initial loop requires a declared induction variable");
        auto stop = strip_conversions(node.child("stopExpr"));
        auto stop_left = stop ? strip_conversions(stop.child("left")) : stop;
        if (stop.kind() != "BinaryOp" ||
            stop_left.kind() != "NamedValue")
            throw std::runtime_error(
                "hardware initial loop requires a simple constant bound");
        auto induction = symbol_name(stop_left);
        auto found = values.find(induction);
        if (found == values.end())
            throw std::runtime_error("uninitialized hardware loop variable: " +
                                     induction);
        auto bound = literal_number(expression(stop.child("right")).name);
        auto condition = [&](uint64_t current) {
            auto operation = stop.text("op");
            if (operation == "LessThan") return current < bound;
            if (operation == "LessThanEqual") return current <= bound;
            if (operation == "GreaterThan") return current > bound;
            if (operation == "GreaterThanEqual") return current >= bound;
            if (operation == "Inequality") return current != bound;
            throw std::runtime_error(
                "unsupported hardware initial loop comparison: " + operation);
        };
        for (size_t iterations = 0; ; ++iterations) {
            if (iterations == 65536)
                throw std::runtime_error(
                    "hardware initial loop exceeds 65536 iterations");
            auto current = literal_number(values.at(induction).name);
            if (!condition(current)) break;
            statement(node.child("body"), enable, sequential);
            auto steps = node.children("steps");
            if (steps.size() != 1 || steps.front().kind() != "UnaryOp")
                throw std::runtime_error(
                    "hardware initial loop requires one increment or decrement");
            auto operation = steps.front().text("op");
            auto operand = strip_conversions(steps.front().child("operand"));
            if (operand.kind() != "NamedValue" ||
                symbol_name(operand) != induction)
                throw std::runtime_error(
                    "hardware initial loop step uses the wrong variable");
            if (operation.ends_with("increment")) ++current;
            else if (operation.ends_with("decrement")) --current;
            else throw std::runtime_error(
                "hardware initial loop step must increment or decrement");
            found->second = {literal(found->second.type, current),
                             found->second.type};
        }
        return;
    }
    if (kind == "ExpressionStatement") {
        auto assignment = node.child("expr");
        if (assignment.kind() == "Call") return;
        if (assignment.kind() != "Assignment")
            throw std::runtime_error("only assignment expression statements are synthesizable");
        auto saved_lvalue = lvalue_reference;
        try {
            lvalue_reference = expression(assignment.child("left"));
        } catch (const UnresolvedValue&) {
            lvalue_reference.reset();
        }
        auto right = expression(assignment.child("right"));
        lvalue_reference = std::move(saved_lvalue);
        assign(assignment.child("left"), right, enable,
               sequential && assignment.boolean("isNonBlocking"));
        return;
    }
    if (kind == "Conditional") {
        auto conditions = node.children("conditions");
        if (conditions.size() != 1) throw std::runtime_error("unsupported conditional statement");
        auto condition = expression(conditions[0].child("expr"));
        auto other = node.child("ifFalse");
        if (!sequential && other) {
            statement(other, enable, false);
            auto when_true = conjunction(*this, enable, condition);
            statement(node.child("ifTrue"), &when_true, false);
            return;
        }
        auto when_true = conjunction(*this, enable, condition);
        statement(node.child("ifTrue"), &when_true, sequential);
        if (other) {
            auto inverted = emit("not", condition.name, "4s<1>");
            auto when_false = conjunction(*this, enable, inverted);
            statement(other, &when_false, sequential);
        }
        return;
    }
    if (kind == "Case") {
        auto selector = expression(node.child("expr"));
        auto fallback = node.child("defaultCase");
        if (!sequential && fallback) statement(fallback, enable, false);
        LoweredValue matched{"4s<1>'0x0", "4s<1>"};
        for (auto item : node.children("items")) {
            LoweredValue item_match{"4s<1>'0x0", "4s<1>"};
            for (auto label : item.children("expressions")) {
                auto value = expression(label);
                auto equal = emit("eq", selector.name + ", " + value.name, "4s<1>");
                item_match = emit("or", item_match.name + ", " + equal.name, "4s<1>");
            }
            matched = emit("or", matched.name + ", " + item_match.name, "4s<1>");
            auto active = conjunction(*this, enable, item_match);
            statement(item.child("stmt"), &active, sequential);
        }
        if (fallback && sequential) {
            auto unmatched = emit("not", matched.name, "4s<1>");
            auto active = conjunction(*this, enable, unmatched);
            statement(fallback, &active, sequential);
        }
        return;
    }
    if (kind == "Empty") return;
    throw std::runtime_error("unsupported semantic statement: " + kind);
}
}
