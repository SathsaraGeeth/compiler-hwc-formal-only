/*
 * compiler/btor2/lib/Lowering/Formal/PropertyProfile.cpp
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

#include "Lowering/Formal/PropertyProfile.h"
#include "frontend/SVA/Model/property.h"

namespace emul::btor2 {
namespace {
using frontend::sva::Property;
using frontend::sva::PropertyKind;
using frontend::sva::Sequence;

bool has_state(frontend::SemanticNode node) {
    if (node.kind() == "ProceduralBlock") {
        auto kind = node.text("procedureKind");
        auto body = node.child("body");
        if (kind == "AlwaysFF" || kind == "AlwaysLatch" ||
            (kind == "Always" && body.kind() == "Timed"))
            return true;
    }
    for (auto member : node.children("members"))
        if (has_state(member)) return true;
    if (node.kind() == "Instance")
        return has_state(node.child("body"));
    return false;
}

bool temporal(const Property& value) {
    if (value.kind == PropertyKind::nexttime ||
        value.kind == PropertyKind::always ||
        value.kind == PropertyKind::eventually ||
        value.kind == PropertyKind::until)
        return true;
    if (value.sequence && value.sequence->kind !=
        frontend::sva::SequenceKind::atom)
        return true;
    return (value.left && temporal(*value.left)) ||
           (value.right && temporal(*value.right));
}

bool unbounded_liveness(const Property& value) {
    if (value.kind == PropertyKind::eventually &&
        value.maximum == Sequence::unbounded)
        return true;
    if (value.kind == PropertyKind::until && value.strong)
        return true;
    return (value.left && unbounded_liveness(*value.left)) ||
           (value.right && unbounded_liveness(*value.right));
}

bool recurrence(const Property& value) {
    if (value.kind == PropertyKind::always && value.left &&
        value.left->kind == PropertyKind::eventually)
        return true;
    if (value.kind == PropertyKind::eventually && value.left &&
        value.left->kind == PropertyKind::always)
        return true;
    return (value.left && recurrence(*value.left)) ||
           (value.right && recurrence(*value.right));
}

bool fairness(const Property& value) {
    return value.kind == PropertyKind::implication && value.left &&
           value.right && recurrence(*value.left) && recurrence(*value.right);
}
}

FormalPropertyFeatures profile(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    std::uint32_t bound) {
    const auto& directive = design.formal_design().directive(top, property);
    FormalPropertyFeatures features;
    features.cover = directive.kind ==
        frontend::sva::DirectiveKind::cover_property;
    features.stateful_design = has_state(design.root());
    features.temporal = temporal(directive.property);
    features.unbounded_liveness = unbounded_liveness(directive.property);
    features.recurrence = recurrence(directive.property);
    features.fairness_implication = fairness(directive.property);
    features.bounded_response = features.temporal &&
        !features.unbounded_liveness && !features.recurrence;
    features.bound = bound;
    return features;
}

} 
