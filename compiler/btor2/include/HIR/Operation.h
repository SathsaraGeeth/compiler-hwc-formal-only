/*
 * compiler/btor2/include/HIR/Operation.h
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
 * Represents one temporal property operation in higher level BTOR2 IR
 */

#pragma once
#include "frontend/SVA/Model/directive.h"
#include <string_view>
#include <vector>

namespace emul::btor2::hir {
enum class Opcode {
    safety,
    guarantee,
    obligation,
    recurrence,
    persistence,
    reactivity
};

struct Operation {
    struct Component {
        Opcode opcode = Opcode::safety;
        frontend::sva::Property formula;
    };

    std::vector<Component> components;
    bool temporal = false;
    frontend::sva::Directive directive;
};

std::string_view spelling(Opcode opcode) noexcept;
} 
