/*
 * compiler/btor2/lib/HIR/Operation.cpp
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

#include "HIR/Operation.h"

namespace emul::btor2::hir {
std::string_view spelling(Opcode opcode) noexcept {
    switch (opcode) {
    case Opcode::safety: return "safety";
    case Opcode::guarantee: return "guarantee";
    case Opcode::obligation: return "obligation";
    case Opcode::recurrence: return "recurrence";
    case Opcode::persistence: return "persistence";
    case Opcode::reactivity: return "reactivity";
    }
    return {};
}
} 
