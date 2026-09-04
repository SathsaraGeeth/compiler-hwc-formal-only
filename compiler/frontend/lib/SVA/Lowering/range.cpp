/*
 * compiler/frontend/lib/SVA/Lowering/range.cpp
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

#include "frontend/SVA/Lowering/range.h"
#include "frontend/SVA/Model/sequence.h"
#include <limits>
#include <stdexcept>

namespace emul::frontend::sva {
namespace {
uint32_t bound(frontend::SemanticValue value) {
    if (!value)
        return 0;
    if (value.kind() == frontend::SemanticValueKind::integer)
        return static_cast<uint32_t>(value.integer());
    if (value.string() == "$")
        return Sequence::unbounded;
    throw std::runtime_error("invalid SVA cycle bound");
}
}

std::pair<uint32_t, uint32_t> lower_range(
    frontend::SemanticNode node) {
    auto minimum = bound(node.value("min"));
    auto maximum = node.value("max") ?
        bound(node.value("max")) : minimum;
    if (maximum != Sequence::unbounded && maximum < minimum)
        throw std::runtime_error("SVA range maximum precedes minimum");
    return {minimum, maximum};
}
}
