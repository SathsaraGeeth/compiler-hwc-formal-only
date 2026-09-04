/*
 * compiler/btor2/lib/Lowering/Formal/harness.cpp
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

#include "Lowering/Formal/harness.h"
#include "eir/include/Lowering/Frontend.h"
#include "eir/include/IR/Verifier.h"
#include "eir/include/Lowering/DeadCode.h"
#include <algorithm>

namespace emul::formal {
namespace {
std::string symbol(frontend::SemanticNode node) {
    auto value = node.text("symbol");
    auto separator = value.rfind(' ');
    return separator == value.npos ? value : value.substr(separator + 1);
}

frontend::SemanticNode find_top(
    frontend::SemanticNode node,
    std::string_view name) {
    if (node.kind() == "Instance" && node.child("body").name() == name)
        return node.child("body");
    for (auto member : node.children("members"))
        if (auto found = find_top(member, name))
            return found;
    return {};
}

void collect_host_flags(frontend::SemanticNode body, Harness& harness) {
    for (auto member : body.children("members")) {
        if (member.kind() != "ProceduralBlock" ||
            member.text("procedureKind") != "AlwaysFF")
            continue;
        auto timed = member.child("body");
        auto event = timed.child("timing");
        auto statement = timed.child("stmt");
        if (event.kind() != "SignalEvent" ||
            event.text("edge") != "PosEdge" ||
            statement.kind() != "ExpressionStatement")
            continue;
        auto assignment = statement.child("expr");
        if (assignment.kind() != "Assignment" ||
            assignment.child("left").kind() != "NamedValue" ||
            assignment.child("right").text("constant") != "1'b1")
            continue;
        harness.host_flags.push_back({
            symbol(assignment.child("left")),
            symbol(event.child("expr"))
        });
    }
}
}

Harness build_harness(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property) {
    Harness harness;
    
    
    harness.directives.push_back(
        design.formal_design().directive(top, property));
    auto environment = design.formal_design().environment(top);
    harness.directives.insert(
        harness.directives.end(), environment.begin(), environment.end());
    harness.hardware = lowering::to_eir(design);
    auto top_body = find_top(design.root(), top);
    auto root = std::find_if(
        harness.hardware.modules.begin(), harness.hardware.modules.end(),
        [&](const eir::Module& module) { return module.name == top; });
    if (!top_body)
        throw std::runtime_error(
            "formal top module not found: " + std::string(top));
    const auto top_members = top_body.children("members");
    const auto has_ports = std::any_of(
        top_members.begin(), top_members.end(),
        [](frontend::SemanticNode member) { return member.kind() == "Port"; });
    
    
    
    
    
    if (root == harness.hardware.modules.end() && has_ports) {
        harness.hardware.modules.push_back(lowering::to_eir(top_body));
        root = std::prev(harness.hardware.modules.end());
    }
    if (root != harness.hardware.modules.end() &&
        std::next(root) != harness.hardware.modules.end())
        std::rotate(root, std::next(root), harness.hardware.modules.end());
    for (auto& module : harness.hardware.modules)
        eir::lowering::eliminate_dead_code(module);
    eir::verify(harness.hardware);
    for (const auto& binding : design.bindings().entries()) {
        if (binding.host.child("body").name() != top)
            continue;
        harness.instance_aliases.push_back({
            binding.hardware.name(), binding.hardware.child("body").name()});
        for (auto connection : binding.hardware.children("connections")) {
            auto expression = connection.child("expr");
            if (expression.kind() == "Assignment")
                expression = expression.child("left");
            if (expression.kind() != "NamedValue")
                continue;
            harness.signal_aliases.push_back({
                symbol(expression), connection.child("port").name()});
        }
    }
    if (top_body)
        collect_host_flags(top_body, harness);
    return harness;
}
}
