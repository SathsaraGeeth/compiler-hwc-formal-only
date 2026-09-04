/*
 * compiler/btor2/lib/Lowering/Formal/monitor.cpp
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

#include "Lowering/Formal/monitor.h"
#include "Lowering/Formal/lower_expression.h"
#include "Lowering/Formal/LTL/buchi_monitor.h"
#include "Lowering/Formal/LTL/spot_automaton.h"
#include <optional>
#include <stdexcept>
#include <vector>

namespace emul::formal {
namespace {
using frontend::sva::Directive;
using frontend::sva::Expression;
using frontend::sva::Property;
using frontend::sva::PropertyKind;
using frontend::sva::Sequence;
using frontend::sva::SequenceKind;

struct DelayedAtom {
    uint32_t minimum = 0;
    uint32_t maximum = 0;
    const Expression* expression = nullptr;
};

struct ImplicationResult {
    Btor2Value violation;
    Btor2Value match;
};

Btor2Value boolean_constant(TransitionSystem& system, bool value) {
    return system.builder.constant(1, value ? "1" : "0");
}

Btor2Value logical_not(Btor2Value value, TransitionSystem& system) {
    return system.builder.unary("not", as_boolean(value, system), 1);
}

Btor2Value logical_and(
    Btor2Value left,
    Btor2Value right,
    TransitionSystem& system) {
    return system.builder.binary("and", left, right, 1);
}

Btor2Value logical_or(
    Btor2Value left,
    Btor2Value right,
    TransitionSystem& system) {
    return system.builder.binary("or", left, right, 1);
}

bool absent(const Expression& expression) {
    return expression.operands.empty() && expression.value.empty() &&
           expression.operation.empty();
}

const Property& unwrap_always(const Property& property) {
    return property.kind == PropertyKind::always ?
        *property.left : property;
}

const Expression* atom(const Property& property) {
    if (property.kind != PropertyKind::sequence || !property.sequence ||
        property.sequence->kind != SequenceKind::atom)
        return nullptr;
    return &property.sequence->expression;
}

bool is_instantaneous(const Property& property) {
    if (atom(property))
        return true;
    if (property.kind == PropertyKind::negation && property.left)
        return is_instantaneous(*property.left);
    if ((property.kind == PropertyKind::conjunction ||
         property.kind == PropertyKind::disjunction ||
         property.kind == PropertyKind::iff ||
         property.kind == PropertyKind::implication) &&
        property.left && property.right)
        return is_instantaneous(*property.left) &&
               is_instantaneous(*property.right);
    return false;
}

std::optional<Btor2Value> instantaneous_property(
    const Property& property,
    TransitionSystem& system,
    Btor2Value sample) {
    if (auto expression = atom(property))
        return as_boolean(
            lower_sva_expression(*expression, system, sample), system);
    if (property.kind == PropertyKind::negation && property.left) {
        auto operand = instantaneous_property(*property.left, system, sample);
        if (operand)
            return logical_not(*operand, system);
    }
    if ((property.kind == PropertyKind::conjunction ||
         property.kind == PropertyKind::disjunction ||
         property.kind == PropertyKind::iff) &&
        property.left && property.right) {
        auto left = instantaneous_property(*property.left, system, sample);
        auto right = instantaneous_property(*property.right, system, sample);
        if (!left || !right)
            return std::nullopt;
        if (property.kind == PropertyKind::conjunction)
            return logical_and(*left, *right, system);
        if (property.kind == PropertyKind::disjunction)
            return logical_or(*left, *right, system);
        if (property.kind == PropertyKind::implication)
            return logical_or(logical_not(*left, system), *right, system);
        return system.builder.binary("eq", *left, *right, 1);
    }
    return std::nullopt;
}

DelayedAtom delayed_atom(const Property& property) {
    if (property.kind != PropertyKind::sequence || !property.sequence)
        return {};
    const auto& sequence = *property.sequence;
    if (sequence.kind == SequenceKind::atom)
        return {0, 0, &sequence.expression};
    if (sequence.kind == SequenceKind::delay && sequence.right &&
        sequence.right->kind == SequenceKind::atom)
        return {sequence.minimum, sequence.maximum,
                &sequence.right->expression};
    return {};
}

Btor2Value sample_event_impl(
    const Directive& directive,
    TransitionSystem& system) {
    const auto cache = "__formal_sample_event." + directive.name;
    if (auto found = system.signals.find(cache);
        found != system.signals.end())
        return found->second;
    if (directive.clock.signal.empty()) {
        auto value = boolean_constant(system, true);
        system.signals[cache] = value;
        return value;
    }
    auto found = system.signals.find(directive.clock.signal);
    if (found == system.signals.end())
        throw std::runtime_error("SVA clock signal not found: " +
                                 directive.clock.signal);
    auto clock = as_boolean(found->second, system);
    if (directive.clock.edge == frontend::sva::Edge::any) {
        auto value = boolean_constant(system, true);
        system.signals[cache] = value;
        return value;
    }
    auto previous = system.builder.state(
        1, directive.name + ".clock_previous");
    auto initialized = system.signals.at("formal_initialized");
    auto initial_clock = system.builder.binary(
        "eq", previous, clock, 1);
    system.builder.constraint(
        system.builder.binary(
            "or", initialized, initial_clock, 1),
        directive.name + ".initial_clock");
    system.builder.next(previous, clock);
    auto value = directive.clock.edge == frontend::sva::Edge::posedge ?
        logical_and(logical_not(previous, system), clock, system) :
        logical_and(previous, logical_not(clock, system), system);
    system.signals[cache] = value;
    return value;
}

Btor2Value disabled(
    const Directive& directive,
    TransitionSystem& system) {
    return absent(directive.disable) ?
        boolean_constant(system, false) :
        as_boolean(lower_sva_expression(directive.disable, system), system);
}

void next_monitor_state(
    Btor2Value state,
    Btor2Value advanced,
    Btor2Value sample,
    Btor2Value reset,
    TransitionSystem& system) {
    auto zero = boolean_constant(system, false);
    auto sampled = system.builder.ternary(
        "ite", sample, advanced, state);
    system.builder.next(
        state, system.builder.ternary("ite", reset, zero, sampled));
}

ImplicationResult implication_monitor(
    const Directive& directive,
    const Property& implication,
    TransitionSystem& system) {
    auto antecedent_expression = atom(*implication.left);
    auto consequent = delayed_atom(*implication.right);
    if (!antecedent_expression || !consequent.expression ||
        consequent.maximum == Sequence::unbounded)
        throw std::runtime_error(
            "this temporal implication requires general automaton lowering");
    auto minimum = consequent.minimum + (implication.overlapped ? 0 : 1);
    auto maximum = consequent.maximum + (implication.overlapped ? 0 : 1);
    auto sample = lower_sample_event(directive, system);
    auto trigger = as_boolean(lower_sva_expression(
        *antecedent_expression, system, sample), system);
    auto condition = as_boolean(lower_sva_expression(
        *consequent.expression, system, sample), system);
    auto disable = disabled(directive, system);
    auto enabled_sample = logical_and(
        sample, logical_not(disable, system), system);
    auto unsatisfied = logical_not(condition, system);

    if (maximum == 0) {
        auto violation = logical_and(trigger, unsatisfied, system);
        auto match = logical_and(trigger, condition, system);
        return {
            logical_and(enabled_sample, violation, system),
            logical_and(enabled_sample, match, system)
        };
    }

    std::vector<Btor2Value> active(maximum + 1);
    auto zero = boolean_constant(system, false);
    for (uint32_t age = 1; age <= maximum; ++age) {
        active[age] = system.builder.state(
            1, directive.name + ".age_" + std::to_string(age));
        system.builder.init(active[age], zero);
    }

    auto match = minimum == 0 ?
        logical_and(trigger, condition, system) : zero;
    auto initial = minimum == 0 ?
        logical_and(trigger, unsatisfied, system) : trigger;
    next_monitor_state(
        active[1], initial, sample, disable, system);
    for (uint32_t age = 1; age < maximum; ++age) {
        auto advanced = active[age];
        if (age >= minimum) {
            match = logical_or(
                match,
                logical_and(active[age], condition, system),
                system);
            advanced = logical_and(advanced, unsatisfied, system);
        }
        next_monitor_state(
            active[age + 1], advanced, sample, disable, system);
    }

    match = logical_or(
        match,
        logical_and(active[maximum], condition, system),
        system);
    auto violation = logical_and(active[maximum], unsatisfied, system);
    return {
        logical_and(enabled_sample, violation, system),
        logical_and(enabled_sample, match, system)
    };
}

}

Btor2Value lower_sample_event(
    const frontend::sva::Directive& directive,
    TransitionSystem& system) {
    return sample_event_impl(directive, system);
}

bool lower_temporal_monitor(
    const Directive& directive,
    TransitionSystem& system) {
    const auto& original = directive.property;
    const auto& core = unwrap_always(original);

    
    
    
    
    if (is_instantaneous(original))
        return false;

    using frontend::sva::DirectiveKind;
    if (directive.kind == DirectiveKind::assume_property ||
        directive.kind == DirectiveKind::restrict_property) {
        ltl::SpotAutomaton translator;
        auto automaton = translator.translate(original, false);
        ltl::BuchiMonitor{}.lower(directive, automaton, system);
        return true;
    }

    
    
    
    if (original.kind == PropertyKind::always) {
        auto sample = lower_sample_event(directive, system);
        if (auto condition = instantaneous_property(core, system, sample)) {
            auto enable = logical_and(
                sample, logical_not(disabled(directive, system), system), system);
            auto violation = logical_and(
                enable, logical_not(*condition, system), system);
            if (directive.kind == frontend::sva::DirectiveKind::assert_property)
                system.builder.bad(violation, directive.name);
            else if (directive.kind ==
                     frontend::sva::DirectiveKind::cover_property)
                system.builder.bad(
                    logical_and(enable, *condition, system), directive.name);
            else
                system.builder.constraint(
                    logical_not(violation, system), directive.name);
            return true;
        }
    }

    
    
    
    if (core.kind != PropertyKind::implication || !core.left || !core.right ||
        !atom(*core.left) ||
        !delayed_atom(*core.right).expression ||
        delayed_atom(*core.right).maximum == Sequence::unbounded) {
        ltl::SpotAutomaton translator;
        auto automaton = translator.translate(
            original, directive.kind == DirectiveKind::assert_property);
        ltl::BuchiMonitor{}.lower(directive, automaton, system);
        return true;
    }

    const auto& property = unwrap_always(original);
    if (property.kind != PropertyKind::implication)
        throw std::runtime_error(
            "unsupported temporal property shape: outer=" +
            std::to_string(static_cast<int>(original.kind)) + ", core=" +
            std::to_string(static_cast<int>(property.kind)));
    auto result = implication_monitor(directive, property, system);
    if (directive.kind == frontend::sva::DirectiveKind::assert_property)
        system.builder.bad(result.violation, directive.name);
    else if (directive.kind ==
             frontend::sva::DirectiveKind::cover_property)
        system.builder.bad(result.match, directive.name);
    else
        system.builder.constraint(
            logical_not(result.violation, system), directive.name);
    return true;
}
}
