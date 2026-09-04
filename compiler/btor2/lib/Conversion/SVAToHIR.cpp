/*
 * compiler/btor2/lib/Conversion/SVAToHIR.cpp
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

#include "Conversion/SVAToHIR.h"

namespace emul::btor2 {
namespace {
using frontend::sva::Property;
using frontend::sva::PropertyKind;

bool is(const Property& property, PropertyKind outer, PropertyKind inner) {
    return property.kind == outer && property.left &&
           property.left->kind == inner;
}

bool obligation(const Property& property) {
    return property.kind == PropertyKind::disjunction && property.left &&
           property.right && property.left->kind == PropertyKind::always &&
           property.right->kind == PropertyKind::eventually;
}

bool reactivity(const Property& property) {
    if (property.kind == PropertyKind::always && property.left &&
        property.left->kind == PropertyKind::implication &&
        property.left->right &&
        property.left->right->kind == PropertyKind::eventually)
        return true;
    return property.kind == PropertyKind::conjunction && property.left &&
           property.right &&
           (reactivity(*property.left) || reactivity(*property.right));
}

bool instantaneous(const Property& property) {
    if (property.kind == PropertyKind::sequence)
        return property.sequence &&
               property.sequence->kind == frontend::sva::SequenceKind::atom;
    if (property.kind == PropertyKind::negation)
        return property.left && instantaneous(*property.left);
    if (property.kind == PropertyKind::conjunction ||
        property.kind == PropertyKind::disjunction ||
        property.kind == PropertyKind::iff)
        return property.left && property.right &&
               instantaneous(*property.left) && instantaneous(*property.right);
    return false;
}

bool temporal(const Property& property) {
    if (property.kind == PropertyKind::always && property.left &&
        instantaneous(*property.left))
        return false;
    if (property.kind != PropertyKind::sequence &&
        property.kind != PropertyKind::negation &&
        property.kind != PropertyKind::conjunction &&
        property.kind != PropertyKind::disjunction &&
        property.kind != PropertyKind::iff)
        return true;
    if (property.sequence &&
        property.sequence->kind != frontend::sva::SequenceKind::atom)
        return true;
    return (property.left && temporal(*property.left)) ||
           (property.right && temporal(*property.right));
}
}

hir::Opcode classify_ltl(const Property& property) {
    if (is(property, PropertyKind::always, PropertyKind::eventually))
        return hir::Opcode::recurrence;
    if (is(property, PropertyKind::eventually, PropertyKind::always))
        return hir::Opcode::persistence;
    if (reactivity(property)) return hir::Opcode::reactivity;
    if (obligation(property)) return hir::Opcode::obligation;
    if (property.kind == PropertyKind::eventually)
        return hir::Opcode::guarantee;
    return hir::Opcode::safety;
}

std::vector<hir::Operation::Component> decompose_ltl(
    const Property& property) {
    if (property.kind == PropertyKind::conjunction && property.left &&
        property.right) {
        auto result = decompose_ltl(*property.left);
        auto right = decompose_ltl(*property.right);
        result.insert(result.end(), right.begin(), right.end());
        return result;
    }
    return {{classify_ltl(property), property}};
}

hir::Operation convert_sva_to_hir(
    const frontend::sva::Directive& directive) {
    return {decompose_ltl(directive.property),
            temporal(directive.property), directive};
}

hir::Module convert_sva_to_hir(
    const std::vector<frontend::sva::Directive>& directives) {
    hir::Module module;
    module.operations.reserve(directives.size());
    for (const auto& directive : directives)
        module.add(convert_sva_to_hir(directive));
    return module;
}
} 
