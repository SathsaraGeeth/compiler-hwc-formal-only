/*
 * compiler/frontend/lib/SVA/Lowering/directive.cpp
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

#include "frontend/SVA/Lowering/directive.h"
#include "frontend/SVA/Lowering/clock.h"
#include "frontend/SVA/Lowering/expression.h"
#include "frontend/SVA/Lowering/property_model.h"
#include "eir/lib/Lowering/Frontend/context.h"
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace emul::frontend::sva {
namespace {
using frontend::SemanticNode;

std::string symbol(SemanticNode node) {
    return lowering::semantic::symbol_name(node);
}

SemanticNode find_top(SemanticNode node, std::string_view name) {
    if (node.kind() == "Instance" && node.child("body").name() == name)
        return node.child("body");
    for (auto member : node.children("members"))
        if (auto found = find_top(member, name))
            return found;
    return {};
}

SemanticNode find_assertion(SemanticNode node, std::string_view name) {
    if (node.kind() == "ConcurrentAssertion") {
        auto expression = node.child("propertySpec").child("expr");
        if (expression.kind() == "AssertionInstance" &&
            symbol(expression) == name)
            return node;
        if (node.name() == name)
            return node;
    }
    for (auto field : node.fields()) {
        if (auto child = node.child(field))
            if (auto found = find_assertion(child, name))
                return found;
        for (auto child : node.children(field))
            if (auto found = find_assertion(child, name))
                return found;
    }
    return {};
}

DirectiveKind directive_kind(std::string_view kind) {
    if (kind == "Assert") return DirectiveKind::assert_property;
    if (kind == "Assume") return DirectiveKind::assume_property;
    if (kind == "CoverProperty") return DirectiveKind::cover_property;
    if (kind == "Restrict") return DirectiveKind::restrict_property;
    throw std::runtime_error("unsupported assertion directive: " +
                             std::string(kind));
}

Clock default_clock(SemanticNode body) {
    for (auto member : body.children("members"))
        if (member.kind() == "ClockingBlock" &&
            member.boolean("isDefault"))
            return lower_clock(member.child("event"));
    return {};
}

SemanticNode unwrap(SemanticNode node, Directive& result) {
    if (node.kind() == "AssertionInstance") {
        for (auto local : node.children("localVars"))
            result.locals.push_back({
                local.name(),
                lowering::semantic::type_width(
                    lowering::semantic::lower_type(local))});
        return unwrap(node.child("body"), result);
    }
    if (node.kind() == "Clocking") {
        result.clock = lower_clock(node.child("clocking"));
        return unwrap(node.child("expr"), result);
    }
    if (node.kind() == "DisableIff") {
        result.disable = lower_expression(node.child("condition"));
        return unwrap(node.child("expr"), result);
    }
    return node;
}

Directive lower_assertion(
    SemanticNode assertion,
    std::string name,
    Clock fallback_clock) {
    Directive result;
    result.name = std::move(name);
    result.kind = directive_kind(assertion.text("assertionKind"));
    auto expression = assertion.child("propertySpec").child("expr");
    result.property = lower_property(unwrap(expression, result));
    if (result.clock.signal.empty())
        result.clock = std::move(fallback_clock);
    return result;
}

void collect_assertions(
    SemanticNode node,
    const Clock& clock,
    std::vector<Directive>& output,
    size_t& next,
    std::unordered_set<int64_t>& visited) {
    if (!node)
        return;
    auto address = node.integer("addr");
    if (address && !visited.insert(address).second)
        return;
    if (node.kind() == "ConcurrentAssertion") {
        auto expression = node.child("propertySpec").child("expr");
        auto name = expression.kind() == "AssertionInstance" ?
            symbol(expression) : node.name();
        if (name.empty())
            name = "assertion_" + std::to_string(next++);
        output.push_back(lower_assertion(node, std::move(name), clock));
        return;
    }
    for (auto field : node.fields()) {
        if (auto child = node.child(field))
            collect_assertions(child, clock, output, next, visited);
        for (auto child : node.children(field))
            collect_assertions(child, clock, output, next, visited);
    }
}

void collect_environment(
    SemanticNode node,
    const Clock& clock,
    std::vector<Directive>& output,
    size_t& next,
    std::unordered_set<int64_t>& visited) {
    if (!node)
        return;
    auto address = node.integer("addr");
    if (address && !visited.insert(address).second)
        return;
    if (node.kind() == "ConcurrentAssertion") {
        auto kind = node.text("assertionKind");
        if (kind != "Assume" && kind != "Restrict")
            return;
        auto expression = node.child("propertySpec").child("expr");
        auto name = expression.kind() == "AssertionInstance" ?
            symbol(expression) : node.name();
        if (name.empty())
            name = "environment_" + std::to_string(next++);
        output.push_back(lower_assertion(node, std::move(name), clock));
        return;
    }
    for (auto field : node.fields()) {
        if (auto child = node.child(field))
            collect_environment(child, clock, output, next, visited);
        for (auto child : node.children(field))
            collect_environment(child, clock, output, next, visited);
    }
}
}

Directive lower_named_directive(
    SemanticNode root,
    std::string_view top,
    std::string_view name) {
    auto body = find_top(root, top);
    if (!body)
        throw std::runtime_error("formal top not found: " + std::string(top));
    auto assertion = find_assertion(body, name);
    if (!assertion)
        throw std::runtime_error("named property not found: " +
                                 std::string(name));
    return lower_assertion(
        assertion, std::string(name), default_clock(body));
}

std::vector<Directive> lower_directives(SemanticNode body) {
    std::vector<Directive> result;
    size_t next = 0;
    std::unordered_set<int64_t> visited;
    collect_assertions(body, default_clock(body), result, next, visited);
    return result;
}

std::vector<Directive> lower_top_directives(
    SemanticNode root,
    std::string_view top) {
    auto body = find_top(root, top);
    if (!body)
        throw std::runtime_error("formal top not found: " +
                                 std::string(top));
    return lower_directives(body);
}

std::vector<Directive> lower_top_environment_directives(
    SemanticNode root,
    std::string_view top) {
    auto body = find_top(root, top);
    if (!body)
        throw std::runtime_error("formal top not found: " +
                                 std::string(top));
    std::vector<Directive> result;
    size_t next = 0;
    std::unordered_set<int64_t> visited;
    collect_environment(body, default_clock(body), result, next, visited);
    return result;
}
}
