/*
 * compiler/btor2/include/Lowering/Formal/LTL/sequence_formula.h
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
 * Converts finite SVA sequences into LTL formulas and match endpoints
 */

#pragma once
#include "frontend/SVA/Model/sequence.h"
#include <functional>
#include <spot/tl/formula.hh>
#include <vector>

namespace emul::formal::ltl {

struct SequenceMatch {
    spot::formula formula;
    uint32_t end = 0;
};

class SequenceFormula {
public:
    using AtomLowerer =
        std::function<spot::formula(const frontend::sva::Expression&)>;

    explicit SequenceFormula(AtomLowerer atom);
    std::vector<SequenceMatch> lower(
        const frontend::sva::Sequence& sequence) const;
    spot::formula lower_sere(
        const frontend::sva::Sequence& sequence) const;

private:
    AtomLowerer atom_;
};

}
