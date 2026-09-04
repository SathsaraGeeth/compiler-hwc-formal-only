/*
 * compiler/btor2/lib/Transform/HIR/ComponentDeduplication.cpp
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

#include "Transform/HIR/ComponentDeduplication.h"
#include <set>
#include <sstream>

namespace emul::btor2::transform {
namespace {
using frontend::sva::Expression;
using frontend::sva::Property;
using frontend::sva::Sequence;

void key(std::ostream& out, const Expression& expression) {
    out << static_cast<int>(expression.kind) << ':' << expression.operation
        << ':' << expression.value << ':' << expression.width << '[';
    for (const auto& operand : expression.operands) key(out, operand);
    out << ']';
}

void key(std::ostream& out, const Sequence* sequence) {
    if (!sequence) { out << '-'; return; }
    out << static_cast<int>(sequence->kind) << ':' << sequence->minimum << ':'
        << sequence->maximum << '{';
    key(out, sequence->expression);
    key(out, sequence->left.get());
    key(out, sequence->right.get());
    for (const auto& item : sequence->match_items) key(out, item);
    out << '}';
}

void key(std::ostream& out, const Property* property) {
    if (!property) { out << '-'; return; }
    out << static_cast<int>(property->kind) << ':' << property->strong << ':'
        << property->overlapped << ':' << property->inclusive << ':'
        << property->followed_by << ':' << property->synchronous << ':'
        << property->minimum << ':' << property->maximum << '{';
    key(out, property->sequence.get());
    key(out, property->left.get());
    key(out, property->right.get());
    key(out, property->condition);
    for (const auto& match : property->case_matches) key(out, match);
    for (const auto& alternative : property->alternatives)
        key(out, alternative.get());
    out << '}';
}
}

bool deduplicate_components(hir::Module& module) {
    bool changed = false;
    for (auto& operation : module.operations) {
        std::set<std::string> seen;
        std::vector<hir::Operation::Component> components;
        for (auto& component : operation.components) {
            std::ostringstream stream;
            stream << static_cast<int>(component.opcode) << ':';
            key(stream, &component.formula);
            if (seen.insert(stream.str()).second)
                components.push_back(std::move(component));
            else
                changed = true;
        }
        operation.components = std::move(components);
    }
    return changed;
}
}
