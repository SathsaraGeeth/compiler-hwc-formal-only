/*
 * compiler/btor2/lib/Transform/IR/PropertyPartition.cpp
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

#include "Transform/IR/PropertyPartition.h"
#include "Transform/IR/DeadCodeElimination.h"

namespace emul::btor2::transform {
namespace {
bool selected(PropertyKind kind, PropertyPartition partition) {
    if (kind == PropertyKind::constraint) return true;
    if (partition == PropertyPartition::safety)
        return kind == PropertyKind::bad || kind == PropertyKind::cover;
    return kind == PropertyKind::fair || kind == PropertyKind::justice;
}
}

bool has_properties(const Module& module, PropertyPartition partition) {
    for (const auto& property : module.properties)
        if (property.kind != PropertyKind::constraint &&
            selected(property.kind, partition))
            return true;
    return false;
}

Module select_properties(const Module& module, PropertyPartition partition) {
    Module result = module;
    std::erase_if(result.properties, [partition](const Property& property) {
        return !selected(property.kind, partition);
    });
    eliminate_dead_code(result);
    return result;
}
}
