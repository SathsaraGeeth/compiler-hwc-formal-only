/*
 * compiler/frontend/lib/SVA/Lowering/property_model.cpp
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

#include "frontend/SVA/Lowering/property_model.h"
#include "frontend/SVA/Lowering/expression.h"
#include "frontend/SVA/Lowering/range.h"
#include "frontend/SVA/Lowering/sequence.h"
#include <memory>
#include <stdexcept>

namespace emul::frontend::sva {
namespace {
using frontend::SemanticNode;

std::shared_ptr<Property> pointer(Property property) {
    return std::make_shared<Property>(std::move(property));
}

Property sequence_property(SemanticNode node) {
    Property result;
    result.kind = PropertyKind::sequence;
    result.sequence = std::make_shared<Sequence>(lower_sequence(node));
    return result;
}

Property unary(SemanticNode node) {
    Property result;
    auto operation = node.text("op");
    if (operation == "Not") result.kind = PropertyKind::negation;
    else if (operation == "NextTime" || operation == "SNextTime")
        result.kind = PropertyKind::nexttime;
    else if (operation == "Always" || operation == "SAlways")
        result.kind = PropertyKind::always;
    else if (operation == "Eventually" || operation == "SEventually")
        result.kind = PropertyKind::eventually;
    else throw std::runtime_error("unsupported unary SVA operator: " + operation);
    result.strong = !operation.empty() && operation.front() == 'S';
    auto [minimum, maximum] = lower_range(node);
    if ((result.kind == PropertyKind::always ||
         result.kind == PropertyKind::eventually) &&
        !node.value("min") && !node.value("max"))
        maximum = Sequence::unbounded;
    result.minimum = minimum;
    result.maximum = maximum;
    result.left = pointer(lower_property(node.child("expr")));
    return result;
}

Property binary(SemanticNode node) {
    Property result;
    auto operation = node.text("op");
    if (operation == "And") result.kind = PropertyKind::conjunction;
    else if (operation == "Or") result.kind = PropertyKind::disjunction;
    else if (operation == "Iff") result.kind = PropertyKind::iff;
    else if (operation == "Implies" ||
             operation == "OverlappedImplication" ||
             operation == "NonOverlappedImplication" ||
             operation == "OverlappedFollowedBy" ||
             operation == "NonOverlappedFollowedBy") {
        result.kind = PropertyKind::implication;
        result.overlapped = operation != "NonOverlappedImplication" &&
                            operation != "NonOverlappedFollowedBy";
        result.followed_by = operation == "OverlappedFollowedBy" ||
                             operation == "NonOverlappedFollowedBy";
    } else if (operation == "Until" || operation == "SUntil" ||
               operation == "UntilWith" || operation == "SUntilWith") {
        result.kind = PropertyKind::until;
        result.strong = operation == "SUntil" || operation == "SUntilWith";
        result.inclusive = operation == "UntilWith" || operation == "SUntilWith";
    } else {
        return sequence_property(node);
    }
    result.left = pointer(lower_property(node.child("left")));
    result.right = pointer(lower_property(node.child("right")));
    return result;
}

Property abort_property(SemanticNode node) {
    Property result;
    result.kind = node.text("action") == "accept" ?
        PropertyKind::accept_on : PropertyKind::reject_on;
    result.synchronous = node.boolean("isSync");
    result.condition = lower_expression(node.child("condition"));
    result.left = pointer(lower_property(node.child("expr")));
    return result;
}
}

Property lower_property(SemanticNode node) {
    if (!node)
        throw std::runtime_error("missing SVA property");
    if (node.kind() == "AssertionInstance")
        return lower_property(node.child("body"));
    if (node.kind() == "Clocking" || node.kind() == "DisableIff")
        return lower_property(node.child("expr"));
    if (node.kind() == "StrongWeak") {
        auto result = lower_property(node.child("expr"));
        result.strong = node.text("strength") == "Strong";
        return result;
    }
    if (node.kind() == "Unary")
        return unary(node);
    if (node.kind() == "Binary")
        return binary(node);
    if (node.kind() == "Abort")
        return abort_property(node);
    if (node.kind() == "Conditional") {
        Property result;
        result.kind = PropertyKind::conditional;
        result.condition = lower_expression(node.child("condition"));
        result.left = pointer(lower_property(node.child("if")));
        if (node.child("else"))
            result.right = pointer(lower_property(node.child("else")));
        return result;
    }
    if (node.kind() == "Case") {
        Property result;
        result.kind = PropertyKind::case_property;
        result.condition = lower_expression(node.child("expr"));
        for (auto item : node.children("items")) {
            auto body = pointer(lower_property(item.child("body")));
            for (auto expression : item.children("expressions")) {
                result.case_matches.push_back(lower_expression(expression));
                result.alternatives.push_back(body);
            }
        }
        if (auto default_case = node.child("defaultCase"))
            result.alternatives.push_back(
                pointer(lower_property(default_case)));
        return result;
    }
    if (node.kind() == "Simple" || node.kind() == "SequenceConcat" ||
        node.kind() == "SequenceWithMatch" || node.kind() == "FirstMatch")
        return sequence_property(node);
    throw std::runtime_error("unsupported SVA property: " + node.kind());
}
}
