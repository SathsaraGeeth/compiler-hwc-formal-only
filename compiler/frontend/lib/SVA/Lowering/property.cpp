/*
 * compiler/frontend/lib/SVA/Lowering/property.cpp
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

#include "frontend/SVA/Lowering/property.h"
#include "frontend/semantic_tree.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace emul::frontend::sva {
namespace {
using frontend::SemanticNode;
using formal::Solver;
struct Value { Solver::BitVector expression; uint32_t width; };
using Environment = std::unordered_map<std::string, Value>;

std::string symbol(SemanticNode node) {
    auto text = node.text("symbol");
    auto space = text.find(' ');
    return space == std::string::npos ? text : text.substr(space + 1);
}
uint32_t width(std::string_view type) {
    static const std::regex range(R"(\[([0-9]+):([0-9]+)\])");
    std::cmatch match;
    if (!std::regex_search(type.begin(), type.end(), match, range)) return 1;
    auto left = std::stoul(match[1].str()), right = std::stoul(match[2].str());
    return static_cast<uint32_t>(left > right ? left - right + 1 : right - left + 1);
}
uint64_t literal_value(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    auto quote = text.find('\'');
    if (quote == std::string::npos) return std::stoull(text, nullptr, 10);
    auto marker = static_cast<char>(std::tolower(text[quote + 1]));
    auto digits = text.substr(quote + 2);
    return std::stoull(digits, nullptr,
        marker == 'h' ? 16 : marker == 'b' ? 2 : marker == 'o' ? 8 : 10);
}
SemanticNode find_top(SemanticNode node, std::string_view name) {
    if (node.kind() == "Instance" && node.child("body").name() == name) return node.child("body");
    for (auto member : node.children("members"))
        if (auto found = find_top(member, name)) return found;
    return {};
}
SemanticNode find_property(SemanticNode node, std::string_view name) {
    if (node.kind() == "AssertionInstance") {
        auto reference = node.text("symbol");
        auto space = reference.find(' ');
        if ((space == reference.npos ? reference : reference.substr(space + 1)) == name)
            return node.child("body").child("expr").child("expr");
    }
    for (auto member : node.children("members"))
        if (auto found = find_property(member, name)) return found;
    for (auto field : {"propertySpec", "expr", "body"})
        if (auto child = node.child(field))
            if (auto found = find_property(child, name)) return found;
    return {};
}

class Lowerer {
public:
    explicit Lowerer(Solver& solver) : solver_(solver) {}

    Value expression(SemanticNode node, Environment& values) {
        while (node.kind() == "Conversion") {
            auto target = width(node.type());
            auto value = expression(node.child("operand"), values);
            if (target > value.width) {
                value.expression = solver_.zero_extend(value.expression, target);
            } else if (target < value.width) {
                value.expression = solver_.slice(value.expression, 0, target);
            }
            value.width = target;
            return value;
        }
        if (node.kind() == "NamedValue") {
            auto name = symbol(node);
            auto found = values.find(name);
            if (found != values.end()) return found->second;
            auto result = Value{solver_.variable(name, width(node.type())), width(node.type())};
            values.emplace(name, result);
            return result;
        }
        if (node.kind() == "IntegerLiteral") {
            auto bits = width(node.type());
            return {solver_.constant(literal_value(node.text("value")), bits), bits};
        }
        if (node.kind() == "Concatenation") {
            auto items = node.children("operands");
            if (items.empty()) throw std::runtime_error("empty formal concatenation");
            auto result = expression(items.front(), values);
            for (size_t index = 1; index < items.size(); ++index) {
                auto low = expression(items[index], values);
                result = {solver_.concat(result.expression, low.expression),
                          result.width + low.width};
            }
            return result;
        }
        if (node.kind() == "BinaryOp" && node.text("op") == "Add") {
            auto left = expression(node.child("left"), values);
            auto right = expression(node.child("right"), values);
            if (left.width != right.width) throw std::runtime_error("formal add width mismatch");
            return {solver_.add(left.expression, right.expression), left.width};
        }
        throw std::runtime_error("unsupported formal expression: " + node.kind());
    }

    Solver::Formula predicate(SemanticNode node, Environment& values) {
        if (node.kind() == "BinaryOp" && node.text("op") == "Equality") {
            auto left = expression(node.child("left"), values);
            auto right = expression(node.child("right"), values);
            if (left.width != right.width) {
                throw std::runtime_error("formal equality width mismatch");
            }
            return solver_.equal(left.expression, right.expression);
        }
        throw std::runtime_error("unsupported formal predicate: " + node.kind());
    }

    void module(SemanticNode body, Environment& values) {
        for (auto member : body.children("members")) {
            if (member.kind() == "Instance") instance(member, values);
            else if (member.kind() == "ContinuousAssign") {
                auto assignment = member.child("assignment");
                assign(assignment.child("left"),
                       expression(assignment.child("right"), values), values);
            }
        }
    }
private:
    Solver& solver_;
    void assign(SemanticNode target, Value value, Environment& values) {
        if (target.kind() == "NamedValue") { values[symbol(target)] = value; return; }
        if (target.kind() != "Concatenation")
            throw std::runtime_error("unsupported formal assignment target");
        auto operands = target.children("operands");
        uint32_t offset = 0;
        for (auto item = operands.rbegin(); item != operands.rend(); ++item) {
            auto item_width = width(item->type());
            values[symbol(*item)] = {
                solver_.slice(value.expression, offset, item_width), item_width};
            offset += item_width;
        }
    }
    void instance(SemanticNode instance, Environment& parent) {
        Environment child;
        struct Output { SemanticNode parent; std::string port; };
        std::vector<Output> outputs;
        for (auto connection : instance.children("connections")) {
            auto port = connection.child("port");
            auto actual = connection.child("expr");
            if (actual.kind() == "Assignment") actual = actual.child("left");
            if (port.text("direction") == "In") child[port.name()] = expression(actual, parent);
            else outputs.push_back({actual, port.name()});
        }
        module(instance.child("body"), child);
        for (auto& output : outputs) {
            auto found = child.find(output.port);
            if (found == child.end()) {
                throw std::runtime_error(
                    "formal output has no driver: " + output.port);
            }
            assign(output.parent, found->second, parent);
        }
    }
};
}

formal::Result lower_and_prove(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    Solver& solver) {
    if (!design) throw std::runtime_error("semantic design is unavailable");
    auto body = find_top(design.root(), top);
    if (!body) throw std::runtime_error("formal top not found: " + std::string(top));
    auto assertion = find_property(body, property);
    if (!assertion) throw std::runtime_error("named property not found: " + std::string(property));
    Environment values;
    Lowerer lowerer(solver);
    lowerer.module(body, values);
    return solver.prove(lowerer.predicate(assertion, values));
}
}
