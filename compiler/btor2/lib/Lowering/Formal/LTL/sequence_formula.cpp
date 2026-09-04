/*
 * compiler/btor2/lib/Lowering/Formal/LTL/sequence_formula.cpp
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

#include "Lowering/Formal/LTL/sequence_formula.h"
#include <algorithm>
#include <stdexcept>

namespace emul::formal::ltl {
namespace {
using frontend::sva::Sequence;
using frontend::sva::SequenceKind;
constexpr uint32_t maximum_expansion = 256;

spot::formula concatenate(std::vector<spot::formula> parts) {
    if (parts.size() == 1) return parts.front();
    return spot::formula::Concat(std::move(parts));
}

spot::formula repeat_range(const spot::formula& unit,
                           uint32_t minimum, uint32_t maximum) {
    std::vector<spot::formula> parts;
    while (minimum > 254) {
        parts.push_back(spot::formula::Star(unit, 254, 254));
        minimum -= 254;
        if (maximum != Sequence::unbounded) maximum -= 254;
    }
    if (maximum == Sequence::unbounded) {
        if (minimum)
            parts.push_back(spot::formula::Star(unit, minimum, minimum));
        parts.push_back(spot::formula::Star(
            unit, 0, spot::formula::unbounded()));
        return concatenate(std::move(parts));
    }
    const auto optional = maximum - minimum;
    if (minimum)
        parts.push_back(spot::formula::Star(unit, minimum, minimum));
    auto remaining = optional;
    while (remaining > 254) {
        parts.push_back(spot::formula::Star(unit, 0, 254));
        remaining -= 254;
    }
    if (remaining || parts.empty())
        parts.push_back(spot::formula::Star(unit, 0, remaining));
    return concatenate(std::move(parts));
}

spot::formula delay(const spot::formula* left, const spot::formula& right,
                    uint32_t minimum, uint32_t maximum) {
    std::vector<spot::formula> alternatives;
    if (left && minimum == 0) {
        alternatives.push_back(spot::formula::Fusion(*left, right));
        minimum = 1;
    }
    if (minimum <= maximum || maximum == Sequence::unbounded) {
        std::vector<spot::formula> parts;
        if (left) parts.push_back(*left);
        const auto gap_minimum = left ? minimum - 1 : minimum;
        const auto gap_maximum = maximum == Sequence::unbounded ?
            Sequence::unbounded : maximum - (left ? 1 : 0);
        parts.push_back(repeat_range(spot::formula::tt(),
                                     gap_minimum, gap_maximum));
        parts.push_back(right);
        alternatives.push_back(concatenate(std::move(parts)));
    }
    return alternatives.size() == 1 ? alternatives.front() :
        spot::formula::OrRat(std::move(alternatives));
}

spot::formula shifted(uint32_t cycles, const spot::formula& formula) {
    return cycles ? spot::formula::X(cycles, formula) : formula;
}

void require_finite_range(const Sequence& sequence, const char* operation) {
    if (sequence.maximum == Sequence::unbounded)
        throw std::runtime_error(std::string("unbounded ") + operation +
                                 " is not finite-LTL-expandable");
    if (sequence.maximum < sequence.minimum ||
        sequence.maximum - sequence.minimum >= maximum_expansion)
        throw std::runtime_error(std::string(operation) +
                                 " expansion is too large");
}
}

SequenceFormula::SequenceFormula(AtomLowerer atom) : atom_(std::move(atom)) {}

spot::formula SequenceFormula::lower_sere(const Sequence& sequence) const {
    if (sequence.kind == SequenceKind::atom)
        return atom_(sequence.expression);
    if (sequence.kind == SequenceKind::match ||
        sequence.kind == SequenceKind::first_match) {
        if (!sequence.left)
            throw std::runtime_error("sequence wrapper has no operand");
        auto result = lower_sere(*sequence.left);
        for (const auto& item : sequence.match_items) {
            result = spot::formula::Fusion(result, atom_(item));
        }
        return sequence.kind == SequenceKind::first_match ?
            spot::formula::first_match(result) : result;
    }
    if (!sequence.left && sequence.kind != SequenceKind::delay)
        throw std::runtime_error("sequence operator has no left operand");
    if ((sequence.kind == SequenceKind::delay ||
         sequence.kind == SequenceKind::concatenation) && sequence.right) {
        auto right = lower_sere(*sequence.right);
        if (!sequence.left)
            return delay(nullptr, right, sequence.minimum, sequence.maximum);
        auto left = lower_sere(*sequence.left);
        return delay(&left, right, sequence.minimum, sequence.maximum);
    }
    if (sequence.kind == SequenceKind::consecutive_repeat)
        return repeat_range(lower_sere(*sequence.left), sequence.minimum,
                            sequence.maximum);
    if (sequence.kind == SequenceKind::nonconsecutive_repeat)
        return spot::formula::sugar_equal(lower_sere(*sequence.left),
                                           sequence.minimum,
                                           sequence.maximum == Sequence::unbounded ?
                                               spot::formula::unbounded() :
                                               sequence.maximum);
    if (sequence.kind == SequenceKind::goto_repeat)
        return spot::formula::sugar_goto(lower_sere(*sequence.left),
                                         sequence.minimum,
                                         sequence.maximum == Sequence::unbounded ?
                                             spot::formula::unbounded() :
                                             sequence.maximum);
    if (!sequence.right)
        throw std::runtime_error("binary sequence operator has no right operand");
    auto left = lower_sere(*sequence.left);
    auto right = lower_sere(*sequence.right);
    if (sequence.kind == SequenceKind::disjunction)
        return spot::formula::OrRat(left, right);
    if (sequence.kind == SequenceKind::conjunction)
        return spot::formula::AndNLM(left, right);
    if (sequence.kind == SequenceKind::intersection)
        return spot::formula::AndRat(left, right);
    if (sequence.kind == SequenceKind::throughout)
        return spot::formula::AndRat(
            spot::formula::Star(left, 0, spot::formula::unbounded()), right);
    if (sequence.kind == SequenceKind::within) {
        auto padding = spot::formula::Star(
            spot::formula::tt(), 0, spot::formula::unbounded());
        return spot::formula::AndRat(
            spot::formula::Concat(
                std::vector<spot::formula>{padding, left, padding}), right);
    }
    throw std::runtime_error("unsupported SVA sequence operator");
}

std::vector<SequenceMatch> SequenceFormula::lower(
    const Sequence& sequence) const {
    if (sequence.kind == SequenceKind::atom)
        return {{atom_(sequence.expression), 0}};

    if (sequence.kind == SequenceKind::match ||
        sequence.kind == SequenceKind::first_match) {
        if (!sequence.match_items.empty())
            throw std::runtime_error(
                "SVA match-item side effects require an attempt engine");
        if (sequence.kind == SequenceKind::first_match)
            throw std::runtime_error(
                "first_match with competing endpoints requires an attempt engine");
        if (!sequence.left)
            throw std::runtime_error("sequence wrapper has no operand");
        return lower(*sequence.left);
    }

    if (sequence.kind == SequenceKind::disjunction) {
        if (!sequence.left || !sequence.right)
            throw std::runtime_error("sequence disjunction has a missing operand");
        auto result = lower(*sequence.left);
        auto right = lower(*sequence.right);
        result.insert(result.end(), right.begin(), right.end());
        return result;
    }

    if (sequence.kind == SequenceKind::conjunction ||
        sequence.kind == SequenceKind::intersection) {
        if (!sequence.left || !sequence.right)
            throw std::runtime_error("sequence composition has a missing operand");
        auto left = lower(*sequence.left);
        auto right = lower(*sequence.right);
        std::vector<SequenceMatch> result;
        for (const auto& lhs : left)
            for (const auto& rhs : right) {
                if (sequence.kind == SequenceKind::intersection &&
                    lhs.end != rhs.end)
                    continue;
                result.push_back({spot::formula::And(lhs.formula, rhs.formula),
                    std::max(lhs.end, rhs.end)});
            }
        if (result.empty())
            throw std::runtime_error("sequence intersection has no common endpoint");
        return result;
    }

    if (sequence.kind == SequenceKind::delay ||
        sequence.kind == SequenceKind::concatenation) {
        if (!sequence.right)
            throw std::runtime_error("sequence delay has no right operand");
        require_finite_range(sequence, "## delay");
        auto right = lower(*sequence.right);
        auto left = sequence.left ? lower(*sequence.left) :
            std::vector<SequenceMatch>{{spot::formula::tt(), 0}};
        std::vector<SequenceMatch> result;
        for (const auto& lhs : left)
            for (uint32_t delay = sequence.minimum;
                 delay <= sequence.maximum; ++delay)
                for (const auto& rhs : right) {
                    auto offset = lhs.end + delay;
                    result.push_back({spot::formula::And(
                        lhs.formula, shifted(offset, rhs.formula)),
                        offset + rhs.end});
                }
        return result;
    }

    if (sequence.kind == SequenceKind::consecutive_repeat) {
        if (!sequence.left)
            throw std::runtime_error("consecutive repetition has no operand");
        require_finite_range(sequence, "consecutive repetition");
        if (sequence.minimum == 0)
            throw std::runtime_error(
                "zero-length repetition requires explicit empty-match semantics");
        auto unit = lower(*sequence.left);
        std::vector<SequenceMatch> result;
        std::vector<SequenceMatch> current{{spot::formula::tt(), 0}};
        for (uint32_t count = 1; count <= sequence.maximum; ++count) {
            std::vector<SequenceMatch> next;
            for (const auto& prefix : current)
                for (const auto& item : unit) {
                    auto offset = count == 1 ? 0 : prefix.end + 1;
                    next.push_back({spot::formula::And(prefix.formula,
                        shifted(offset, item.formula)), offset + item.end});
                }
            current = std::move(next);
            if (count >= sequence.minimum)
                result.insert(result.end(), current.begin(), current.end());
            if (result.size() >= maximum_expansion ||
                current.size() >= maximum_expansion)
                throw std::runtime_error(
                    "consecutive repetition expansion is too large");
        }
        return result;
    }

    if (sequence.kind == SequenceKind::throughout) {
        if (!sequence.left || !sequence.right ||
            sequence.left->kind != SequenceKind::atom)
            throw std::runtime_error(
                "throughout currently requires an atomic left operand");
        auto condition = atom_(sequence.left->expression);
        auto right = lower(*sequence.right);
        for (auto& match : right)
            for (uint32_t cycle = 0; cycle <= match.end; ++cycle)
                match.formula = spot::formula::And(
                    match.formula, shifted(cycle, condition));
        return right;
    }

    if (sequence.kind == SequenceKind::within) {
        if (!sequence.left || !sequence.right)
            throw std::runtime_error("within has a missing operand");
        auto inner = lower(*sequence.left);
        auto envelope = lower(*sequence.right);
        std::vector<SequenceMatch> result;
        for (const auto& outer : envelope)
            for (const auto& item : inner) {
                if (item.end > outer.end)
                    continue;
                for (uint32_t offset = 0; offset + item.end <= outer.end; ++offset)
                    result.push_back({spot::formula::And(
                        outer.formula, shifted(offset, item.formula)), outer.end});
            }
        if (result.empty())
            throw std::runtime_error("within sequence cannot fit its envelope");
        return result;
    }

    if (sequence.kind == SequenceKind::nonconsecutive_repeat ||
        sequence.kind == SequenceKind::goto_repeat)
        throw std::runtime_error(
            "nonconsecutive and goto repetition require an attempt engine");
    throw std::runtime_error("sequence operator is not supported inside general LTL");
}

}
