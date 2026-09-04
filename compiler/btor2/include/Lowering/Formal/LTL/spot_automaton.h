/*
 * compiler/btor2/include/Lowering/Formal/LTL/spot_automaton.h
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
 * Converts SVA properties into automata using Spot
 */

#pragma once

#include "frontend/SVA/Model/property.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace emul::formal::ltl {

struct Guard {
    enum class Kind { constant, atom, negation, conjunction, disjunction };
    Kind kind = Kind::constant;
    bool constant = false;
    uint32_t atom = 0;
    std::shared_ptr<Guard> left;
    std::shared_ptr<Guard> right;
};

struct Transition {
    uint32_t source = 0;
    uint32_t destination = 0;
    std::shared_ptr<Guard> guard;
};

struct Automaton {
    std::string formula;
    bool deterministic = false;
    uint32_t initial = 0;
    uint32_t states = 0;
    std::vector<frontend::sva::Expression> atoms;
    std::vector<bool> accepting;
    std::vector<Transition> transitions;
};

class SpotAutomaton {
public:
    Automaton translate(
        const frontend::sva::Property& property,
        bool complement) const;
};

}
