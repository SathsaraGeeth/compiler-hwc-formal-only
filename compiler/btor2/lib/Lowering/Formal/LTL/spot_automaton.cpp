/*
 * compiler/btor2/lib/Lowering/Formal/LTL/spot_automaton.cpp
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

#include "Lowering/Formal/LTL/spot_automaton.h"
#include "Lowering/Formal/LTL/sequence_formula.h"
#include <bddx.h>
#include <spot/tl/formula.hh>
#include <spot/tl/print.hh>
#include <spot/twaalgos/sbacc.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/translate.hh>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace emul::formal::ltl {
namespace {
using frontend::sva::Property;
using frontend::sva::PropertyKind;
using frontend::sva::SequenceKind;

class FormulaBuilder {
public:
    spot::formula atom(const frontend::sva::Expression& expression) {
        auto index = atoms.size();
        atoms.push_back(expression);
        return spot::formula::ap("hwc_ap_" + std::to_string(index));
    }

    spot::formula lower(const Property& property) {
        if (property.kind == PropertyKind::sequence) {
            if (!property.sequence)
                throw std::runtime_error("temporal property has no sequence");
            auto sequence = SequenceFormula([this](const auto& expression) {
                return atom(expression);
            }).lower_sere(*property.sequence);
            return spot::formula::Closure(sequence);
        }
        if (property.kind == PropertyKind::conditional) {
            if (!property.left)
                throw std::runtime_error(
                    "conditional temporal property has no true branch");
            auto condition = atom(property.condition);
            auto when_true = lower(*property.left);
            
            
            auto when_false = property.right ?
                lower(*property.right) : spot::formula::tt();
            return spot::formula::Or(
                spot::formula::And(condition, when_true),
                spot::formula::And(
                    spot::formula::Not(condition), when_false));
        }
        if (property.kind == PropertyKind::case_property) {
            if (property.case_matches.size() > property.alternatives.size())
                throw std::runtime_error("case property has missing alternatives");
            std::vector<spot::formula> alternatives;
            std::vector<spot::formula> matches;
            for (size_t index = 0; index < property.case_matches.size(); ++index) {
                frontend::sva::Expression equality{
                    frontend::sva::ExpressionKind::binary,
                    "Equality", {}, 1,
                    {property.condition, property.case_matches[index]}};
                auto match = atom(equality);
                matches.push_back(match);
                alternatives.push_back(spot::formula::And(
                    match, lower(*property.alternatives[index])));
            }
            if (property.alternatives.size() > property.case_matches.size()) {
                auto no_match = matches.empty() ? spot::formula::tt() :
                    spot::formula::Not(spot::formula::Or(matches));
                alternatives.push_back(spot::formula::And(no_match,
                    lower(*property.alternatives.back())));
            } else if (!matches.empty()) {
                alternatives.push_back(
                    spot::formula::Not(spot::formula::Or(matches)));
            }
            return spot::formula::Or(alternatives);
        }
        if (property.kind == PropertyKind::accept_on ||
            property.kind == PropertyKind::reject_on) {
            if (!property.left)
                throw std::runtime_error("abort property has no operand");
            auto condition = atom(property.condition);
            auto body = lower(*property.left);
            return property.kind == PropertyKind::accept_on ?
                spot::formula::Or(condition, body) :
                spot::formula::And(spot::formula::Not(condition), body);
        }
        if (!property.left)
            throw std::runtime_error("temporal property has no operand");
        if (property.kind == PropertyKind::implication && property.left &&
            property.left->kind == PropertyKind::sequence &&
            property.left->sequence && property.right) {
            auto sequence = SequenceFormula([this](const auto& expression) {
                return atom(expression);
            }).lower_sere(*property.left->sequence);
            auto consequent = lower(*property.right);
            if (!property.overlapped)
                consequent = spot::formula::X(consequent);
            return spot::formula::UConcat(sequence, consequent);
        }
        auto left = lower(*property.left);
        if (property.kind == PropertyKind::negation)
            return spot::formula::Not(left);
        if (property.kind == PropertyKind::nexttime)
            return spot::formula::X(property.minimum ? property.minimum : 1, left);
        if (property.kind == PropertyKind::always)
            return property.maximum == UINT32_MAX ? spot::formula::G(left) :
                spot::formula::G(property.minimum, property.maximum, left);
        if (property.kind == PropertyKind::eventually)
            return property.maximum == UINT32_MAX ? spot::formula::F(left) :
                spot::formula::F(property.minimum, property.maximum, left);
        if (!property.right)
            throw std::runtime_error("binary temporal property has no right operand");
        auto right = lower(*property.right);
        if (property.kind == PropertyKind::conjunction)
            return spot::formula::And(left, right);
        if (property.kind == PropertyKind::disjunction)
            return spot::formula::Or(left, right);
        if (property.kind == PropertyKind::implication)
            return spot::formula::Implies(left, right);
        if (property.kind == PropertyKind::iff)
            return spot::formula::Equiv(left, right);
        if (property.kind == PropertyKind::until) {
            if (property.inclusive)
                right = spot::formula::And(left, right);
            return property.strong ? spot::formula::U(left, right) :
                spot::formula::W(left, right);
        }
        throw std::runtime_error("property operator is not LTL-compatible");
    }

    std::vector<frontend::sva::Expression> atoms;

};

std::shared_ptr<Guard> bdd_guard(
    bdd value,
    const spot::bdd_dict_ptr& dictionary,
    const std::unordered_map<std::string, uint32_t>& atoms) {
    auto result = std::make_shared<Guard>();
    if (value == bddtrue || value == bddfalse) {
        result->constant = value == bddtrue;
        return result;
    }
    auto variable = bdd_var(value);
    auto name = dictionary->bdd_map.at(variable).f.ap_name();
    auto found = atoms.find(name);
    if (found == atoms.end())
        throw std::runtime_error("Spot emitted an unknown atomic proposition");
    auto atom = std::make_shared<Guard>();
    atom->kind = Guard::Kind::atom;
    atom->atom = found->second;
    auto positive = std::make_shared<Guard>();
    positive->kind = Guard::Kind::conjunction;
    positive->left = atom;
    positive->right = bdd_guard(bdd_high(value), dictionary, atoms);
    auto negative_atom = std::make_shared<Guard>();
    negative_atom->kind = Guard::Kind::negation;
    negative_atom->left = atom;
    auto negative = std::make_shared<Guard>();
    negative->kind = Guard::Kind::conjunction;
    negative->left = negative_atom;
    negative->right = bdd_guard(bdd_low(value), dictionary, atoms);
    result->kind = Guard::Kind::disjunction;
    result->left = negative;
    result->right = positive;
    return result;
}
}

Automaton SpotAutomaton::translate(
    const frontend::sva::Property& property,
    bool complement) const {
    FormulaBuilder builder;
    auto formula = builder.lower(property);
    if (complement)
        formula = spot::formula::Not(formula);
    spot::translator translator;
    translator.set_type(spot::postprocessor::BA);
    auto graph = spot::sbacc(translator.run(formula));

    Automaton result;
    result.formula = spot::str_psl(formula);
    result.deterministic = spot::is_deterministic(graph);
    result.initial = graph->get_init_state_number();
    result.states = graph->num_states();
    result.atoms = std::move(builder.atoms);
    result.accepting.resize(result.states);
    std::unordered_map<std::string, uint32_t> atom_names;
    for (uint32_t i = 0; i < result.atoms.size(); ++i)
        atom_names.emplace("hwc_ap_" + std::to_string(i), i);
    for (uint32_t state = 0; state < result.states; ++state) {
        result.accepting[state] = graph->state_is_accepting(state);
        for (const auto& edge : graph->out(state))
            result.transitions.push_back({
                state, edge.dst,
                bdd_guard(edge.cond, graph->get_dict(), atom_names)});
    }
    return result;
}

}
