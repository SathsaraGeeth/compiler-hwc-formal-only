/*
 * compiler/frontend/lib/SVA/Lowering/sequence.cpp
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

#include "frontend/SVA/Lowering/sequence.h"
#include "frontend/SVA/Lowering/expression.h"
#include "frontend/SVA/Lowering/range.h"
#include <memory>
#include <stdexcept>

namespace emul::frontend::sva {
namespace {
using frontend::SemanticNode;

std::shared_ptr<Sequence> pointer(Sequence sequence) {
    return std::make_shared<Sequence>(std::move(sequence));
}

Sequence repeat(Sequence sequence, SemanticNode repetition) {
    if (!repetition)
        return sequence;
    auto [minimum, maximum] = lower_range(repetition);
    auto kind = repetition.text("kind");
    Sequence result;
    result.kind = kind == "Consecutive" ?
        SequenceKind::consecutive_repeat :
        kind == "Nonconsecutive" ?
        SequenceKind::nonconsecutive_repeat :
        SequenceKind::goto_repeat;
    if (kind != "Consecutive" && kind != "Nonconsecutive" &&
        kind != "GoTo")
        throw std::runtime_error("unsupported SVA repetition: " + kind);
    result.minimum = minimum;
    result.maximum = maximum;
    result.left = pointer(std::move(sequence));
    return result;
}

Sequence simple(SemanticNode node) {
    Sequence result;
    result.kind = SequenceKind::atom;
    result.expression = lower_expression(node.child("expr"));
    return repeat(std::move(result), node.child("repetition"));
}

Sequence with_match(SemanticNode node) {
    Sequence result;
    result.kind = SequenceKind::match;
    result.left = pointer(lower_sequence(node.child("expr")));
    for (auto item : node.children("matchItems"))
        result.match_items.push_back(lower_expression(item));
    return result;
}

Sequence binary(SemanticNode node) {
    auto operation = node.text("op");
    Sequence result;
    if (operation == "And") result.kind = SequenceKind::conjunction;
    else if (operation == "Or") result.kind = SequenceKind::disjunction;
    else if (operation == "Intersect") result.kind = SequenceKind::intersection;
    else if (operation == "Throughout") result.kind = SequenceKind::throughout;
    else if (operation == "Within") result.kind = SequenceKind::within;
    else throw std::runtime_error(
        "property operator used where sequence is required: " + operation);
    result.left = pointer(lower_sequence(node.child("left")));
    result.right = pointer(lower_sequence(node.child("right")));
    return result;
}

Sequence concatenate(SemanticNode node) {
    std::shared_ptr<Sequence> result;
    for (auto element : node.children("elements")) {
        auto [minimum, maximum] = lower_range(element);
        Sequence delay;
        delay.kind = SequenceKind::delay;
        delay.minimum = minimum;
        delay.maximum = maximum;
        delay.right = pointer(lower_sequence(element.child("sequence")));
        if (!result) {
            if (minimum == 0 && maximum == 0)
                result = delay.right;
            else
                result = pointer(std::move(delay));
            continue;
        }
        delay.left = std::move(result);
        delay.kind = SequenceKind::concatenation;
        result = pointer(std::move(delay));
    }
    if (!result)
        throw std::runtime_error("empty SVA sequence concatenation");
    return std::move(*result);
}
}

Sequence lower_sequence(SemanticNode node) {
    if (!node)
        throw std::runtime_error("missing SVA sequence");
    if (node.kind() == "AssertionInstance")
        return lower_sequence(node.child("body"));
    if (node.kind() == "Simple")
        return simple(node);
    if (node.kind() == "SequenceWithMatch")
        return with_match(node);
    if (node.kind() == "SequenceConcat")
        return concatenate(node);
    if (node.kind() == "Binary")
        return binary(node);
    if (node.kind() == "FirstMatch") {
        Sequence result;
        result.kind = SequenceKind::first_match;
        result.left = pointer(lower_sequence(node.child("seq")));
        for (auto item : node.children("matchItems"))
            result.match_items.push_back(lower_expression(item));
        return result;
    }
    if (node.kind() == "Clocking" || node.kind() == "StrongWeak")
        return lower_sequence(node.child("expr"));
    throw std::runtime_error("unsupported SVA sequence: " + node.kind());
}
}
