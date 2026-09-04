/*
 * compiler/frontend/lib/SVA/Lowering/clock.cpp
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

#include "frontend/SVA/Lowering/clock.h"
#include "eir/lib/Lowering/Frontend/context.h"

namespace emul::frontend::sva {
namespace {
frontend::SemanticNode find_signal(frontend::SemanticNode node) {
    if (!node)
        return {};
    if (node.kind() == "NamedValue")
        return node;
    for (auto field : node.fields()) {
        if (auto child = node.child(field))
            if (auto found = find_signal(child))
                return found;
        for (auto child : node.children(field))
            if (auto found = find_signal(child))
                return found;
    }
    return {};
}
}

Clock lower_clock(frontend::SemanticNode node) {
    Clock result;
    auto signal = find_signal(node);
    if (signal)
        result.signal = lowering::semantic::symbol_name(signal);
    auto edge = node.text("edge");
    if (edge == "PosEdge") result.edge = Edge::posedge;
    else if (edge == "NegEdge") result.edge = Edge::negedge;
    return result;
}
}
