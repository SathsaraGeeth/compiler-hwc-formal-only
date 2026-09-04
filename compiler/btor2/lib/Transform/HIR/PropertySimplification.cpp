/*
 * compiler/btor2/lib/Transform/HIR/PropertySimplification.cpp
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

#include "Transform/HIR/PropertySimplification.h"

namespace emul::btor2::transform {
namespace {
using frontend::sva::Property;
using frontend::sva::PropertyKind;

bool simplify(Property& property) {
    bool changed = false;
    if (property.left) changed |= simplify(*property.left);
    if (property.right) changed |= simplify(*property.right);
    for (auto& alternative : property.alternatives)
        if (alternative) changed |= simplify(*alternative);

    if (property.kind == PropertyKind::negation && property.left &&
        property.left->kind == PropertyKind::negation &&
        property.left->left) {
        property = *property.left->left;
        return true;
    }
    if ((property.kind == PropertyKind::always ||
         property.kind == PropertyKind::eventually) &&
        property.left && property.left->kind == property.kind &&
        property.left->left) {
        property.left = property.left->left;
        return true;
    }
    return changed;
}
}

bool simplify_properties(hir::Module& module) {
    bool changed = false;
    for (auto& operation : module.operations) {
        changed |= simplify(operation.directive.property);
        for (auto& component : operation.components)
            changed |= simplify(component.formula);
    }
    return changed;
}
}
