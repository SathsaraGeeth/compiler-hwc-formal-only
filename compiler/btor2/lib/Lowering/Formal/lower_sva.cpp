/*
 * compiler/btor2/lib/Lowering/Formal/lower_sva.cpp
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

#include "Lowering/Formal/lower_sva.h"
#include "Lowering/Formal/lower_expression.h"
#include "Lowering/Formal/monitor.h"
#include <stdexcept>

namespace emul::formal {
namespace {
Btor2Value negate(Btor2Value value, TransitionSystem& system) {
    return system.builder.unary("not", as_boolean(value, system), 1);
}

Btor2Value sequence(
    const frontend::sva::Sequence& value,
    TransitionSystem& system) {
    using frontend::sva::SequenceKind;
    if (value.kind != SequenceKind::atom)
        throw std::runtime_error(
            "temporal sequence requires monitor lowering");
    return as_boolean(lower_sva_expression(value.expression, system), system);
}

Btor2Value property(
    const frontend::sva::Property& value,
    TransitionSystem& system) {
    using frontend::sva::PropertyKind;
    if (value.kind == PropertyKind::sequence)
        return sequence(*value.sequence, system);
    if (value.kind == PropertyKind::negation)
        return negate(property(*value.left, system), system);
    if (value.kind == PropertyKind::conjunction ||
        value.kind == PropertyKind::disjunction ||
        value.kind == PropertyKind::iff ||
        value.kind == PropertyKind::implication) {
        auto left = property(*value.left, system);
        auto right = property(*value.right, system);
        if (value.kind == PropertyKind::conjunction)
            return system.builder.binary("and", left, right, 1);
        if (value.kind == PropertyKind::disjunction)
            return system.builder.binary("or", left, right, 1);
        if (value.kind == PropertyKind::iff)
            return system.builder.binary("eq", left, right, 1);
        return system.builder.binary(
            "or", negate(left, system), right, 1);
    }
    throw std::runtime_error("temporal property requires monitor lowering");
}
}

void lower_sva_directive(
    const frontend::sva::Directive& directive,
    TransitionSystem& system) {
    if (lower_temporal_monitor(directive, system))
        return;
    auto sample = lower_sample_event(directive, system);
    auto enabled = directive.disable.operands.empty() &&
                   directive.disable.value.empty() &&
                   directive.disable.operation.empty() ?
        sample :
        system.builder.binary(
            "and", sample,
            negate(as_boolean(
                lower_sva_expression(directive.disable, system), system),
                system), 1);
    auto holds = property(directive.property, system);
    auto violation = system.builder.binary(
        "and", enabled, negate(holds, system), 1);
    using frontend::sva::DirectiveKind;
    if (directive.kind == DirectiveKind::assert_property)
        system.builder.bad(violation, directive.name);
    else if (directive.kind == DirectiveKind::assume_property ||
             directive.kind == DirectiveKind::restrict_property)
        system.builder.constraint(
            system.builder.binary(
                "or", negate(enabled, system), holds, 1),
            directive.name);
    else
        system.builder.bad(
            system.builder.binary("and", enabled, holds, 1),
            directive.name);
}
}
