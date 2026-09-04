/*
 * compiler/btor2/lib/Lowering/Formal/LTL/buchi_monitor.cpp
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

#include "Lowering/Formal/LTL/buchi_monitor.h"
#include "Lowering/Formal/lower_expression.h"
#include "Lowering/Formal/monitor.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace emul::formal::ltl {
namespace {
Btor2Value constant(TransitionSystem& system, bool value) {
    return system.builder.constant(1, value ? "1" : "0");
}

Btor2Value logical_not(TransitionSystem& system, Btor2Value value) {
    return system.builder.unary("not", as_boolean(value, system), 1);
}

Btor2Value logical_and(
    TransitionSystem& system, Btor2Value left, Btor2Value right) {
    return system.builder.binary("and", left, right, 1);
}

Btor2Value logical_or(
    TransitionSystem& system, Btor2Value left, Btor2Value right) {
    return system.builder.binary("or", left, right, 1);
}

Btor2Value guard(
    const Guard& value,
    const std::vector<Btor2Value>& atoms,
    TransitionSystem& system) {
    if (value.kind == Guard::Kind::constant)
        return constant(system, value.constant);
    if (value.kind == Guard::Kind::atom)
        return atoms.at(value.atom);
    auto left = guard(*value.left, atoms, system);
    if (value.kind == Guard::Kind::negation)
        return logical_not(system, left);
    auto right = guard(*value.right, atoms, system);
    return value.kind == Guard::Kind::conjunction ?
        logical_and(system, left, right) : logical_or(system, left, right);
}

bool mentions(const Guard& value, uint32_t atom) {
    if (value.kind == Guard::Kind::atom) return value.atom == atom;
    if (value.kind == Guard::Kind::constant) return false;
    return (value.left && mentions(*value.left, atom)) ||
           (value.right && mentions(*value.right, atom));
}

std::string bits(uint32_t width, uint64_t value) {
    std::string result(width, '0');
    for (uint32_t bit = 0; bit < width; ++bit)
        if (value & (uint64_t{1} << bit))
            result[width - bit - 1] = '1';
    return result;
}
}

void BuchiMonitor::lower(
    const frontend::sva::Directive& directive,
    const Automaton& automaton,
    TransitionSystem& system) const {
    if (!automaton.states || automaton.transitions.empty()) {
        system.builder.justice({constant(system, false)}, directive.name);
        return;
    }
    auto sample = lower_sample_event(directive, system);
    auto disable = directive.disable.operands.empty() &&
                   directive.disable.value.empty() &&
                   directive.disable.operation.empty() ?
        constant(system, false) :
        as_boolean(lower_sva_expression(directive.disable, system), system);
    auto enabled = logical_and(system, sample, logical_not(system, disable));
    struct MatchAssignment {
        uint32_t atom = 0;
        Btor2Value state;
        frontend::sva::Expression value;
    };
    std::vector<MatchAssignment> assignments;
    std::unordered_map<std::string, Btor2Value> local_by_name;
    for (uint32_t index = 0; index < automaton.atoms.size(); ++index) {
        const auto& expression = automaton.atoms[index];
        if (expression.kind != frontend::sva::ExpressionKind::assignment)
            continue;
        if (expression.operands.size() != 2 ||
            expression.operands.front().kind !=
                frontend::sva::ExpressionKind::signal)
            throw std::runtime_error(
                "SVA match assignment requires a named local variable");
        const auto& local = expression.operands.front();
        Btor2Value state;
        auto found = local_by_name.find(local.value);
        if (found == local_by_name.end()) {
            state = system.builder.state(local.width,
                directive.name + ".local." + local.value);
            system.builder.init(state,
                system.builder.constant(local.width, std::string(local.width, '0')));
            local_by_name.emplace(local.value, state);
            system.signals[local.value] = state;
        } else {
            state = found->second;
        }
        assignments.push_back({index, state, expression.operands[1]});
    }
    std::vector<Btor2Value> atoms;
    for (const auto& expression : automaton.atoms) {
        if (expression.kind == frontend::sva::ExpressionKind::assignment)
            atoms.push_back(constant(system, true));
        else
            atoms.push_back(as_boolean(
                lower_sva_expression(expression, system, sample), system));
    }

    auto choice_width = automaton.transitions.size() <= 1 ? 1 :
        32 - __builtin_clz(static_cast<unsigned>(
            automaton.transitions.size() - 1));
    Btor2Value choice;
    if (!automaton.deterministic)
        choice = system.builder.input(choice_width, directive.name + ".edge");
    auto zero = constant(system, false);
    auto one = constant(system, true);
    std::vector<Btor2Value> active;
    active.reserve(automaton.states);
    for (uint32_t state = 0; state < automaton.states; ++state) {
        auto bit = system.builder.state(1,
            directive.name + ".state_" + std::to_string(state));
        system.builder.init(bit,
            state == automaton.initial ? one : zero);
        active.push_back(bit);
    }

    std::vector<Btor2Value> selected;
    selected.reserve(automaton.transitions.size());
    for (uint32_t index = 0; index < automaton.transitions.size(); ++index) {
        const auto& edge = automaton.transitions[index];
        auto chosen = one;
        if (!automaton.deterministic) {
            auto choice_value = system.builder.constant(
                choice_width, bits(choice_width, index));
            chosen = system.builder.binary("eq", choice, choice_value, 1);
        }
        selected.push_back(logical_and(system, active[edge.source],
            logical_and(system, chosen, guard(*edge.guard, atoms, system))));
    }
    auto transition_taken = zero;
    for (auto value : selected)
        transition_taken = logical_or(system, transition_taken, value);
    system.builder.constraint(
        logical_or(system, logical_not(system, enabled), transition_taken),
        directive.name + ".transition");

    std::unordered_map<uint64_t, Btor2Value> local_updates;
    std::unordered_map<uint64_t, Btor2Value> local_states;
    for (const auto& assignment : assignments) {
        auto write = zero;
        for (uint32_t index = 0; index < automaton.transitions.size(); ++index)
            if (mentions(*automaton.transitions[index].guard, assignment.atom))
                write = logical_or(system, write, selected[index]);
        auto value = lower_sva_expression(assignment.value, system, sample);
        if (value.width != assignment.state.width)
            throw std::runtime_error("SVA match assignment width mismatch");
        auto current = local_updates.contains(assignment.state.node) ?
            local_updates.at(assignment.state.node) : assignment.state;
        local_updates[assignment.state.node] = system.builder.ternary(
            "ite", write, value, current);
        local_states[assignment.state.node] = assignment.state;
    }
    for (const auto& [node, updated] : local_updates) {
        const auto state = local_states.at(node);
        system.builder.next(state,
            system.builder.ternary(
                "ite", disable,
                system.builder.constant(state.width,
                    std::string(state.width, '0')),
                updated));
    }

    for (uint32_t destination = 0; destination < automaton.states; ++destination) {
        auto next_active = zero;
        for (uint32_t index = 0; index < automaton.transitions.size(); ++index)
            if (automaton.transitions[index].destination == destination)
                next_active = logical_or(system, next_active, selected[index]);
        auto advanced = system.builder.ternary(
            "ite", enabled, next_active, active[destination]);
        auto reset = destination == automaton.initial ? one : zero;
        system.builder.next(active[destination],
            system.builder.ternary("ite", disable, reset, advanced));
    }

    auto accepting = zero;
    for (uint32_t state = 0; state < automaton.states; ++state)
        if (automaton.accepting[state])
            accepting = logical_or(system, accepting, active[state]);
    system.builder.justice(
        {logical_and(system, enabled, accepting)}, directive.name);
}

}
