/*
 * compiler/btor2/lib/HIR/Printer.cpp
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

#include "HIR/Printer.h"
#include <ostream>
#include <sstream>

namespace emul::btor2::hir {
namespace {
using frontend::sva::Expression;
using frontend::sva::Property;
using frontend::sva::PropertyKind;
using frontend::sva::Sequence;
using frontend::sva::SequenceKind;

std::string expression(const Expression& value);

std::string arguments(const std::vector<Expression>& operands) {
    std::ostringstream output;
    for (std::size_t index = 0; index < operands.size(); ++index) {
        if (index) output << ", ";
        output << expression(operands[index]);
    }
    return output.str();
}

std::string expression(const Expression& value) {
    using frontend::sva::ExpressionKind;
    if (value.kind == ExpressionKind::signal ||
        value.kind == ExpressionKind::constant)
        return value.value;
    if (value.kind == ExpressionKind::unary && value.operands.size() == 1)
        return value.operation + "(" + expression(value.operands[0]) + ")";
    if (value.kind == ExpressionKind::binary && value.operands.size() == 2)
        return "(" + expression(value.operands[0]) + " " + value.operation +
               " " + expression(value.operands[1]) + ")";
    if (value.kind == ExpressionKind::conditional &&
        value.operands.size() == 3)
        return "(" + expression(value.operands[0]) + " ? " +
               expression(value.operands[1]) + " : " +
               expression(value.operands[2]) + ")";
    if (value.kind == ExpressionKind::assignment && value.operands.size() == 2)
        return expression(value.operands[0]) +
               (value.operation == "nonblocking" ? " <= " : " = ") +
               expression(value.operands[1]);
    return value.operation + "(" + arguments(value.operands) + ")";
}

std::string sequence(const Sequence& value) {
    if (value.kind == SequenceKind::atom)
        return expression(value.expression);
    if (value.kind == SequenceKind::delay ||
        value.kind == SequenceKind::concatenation) {
        auto maximum = value.maximum == Sequence::unbounded ? "$" :
            std::to_string(value.maximum);
        auto left = value.left ? sequence(*value.left) + " " : std::string{};
        return "(" + left + "##[" +
               std::to_string(value.minimum) + ":" + maximum + "] " +
               sequence(*value.right) + ")";
    }
    if (value.kind == SequenceKind::consecutive_repeat ||
        value.kind == SequenceKind::nonconsecutive_repeat ||
        value.kind == SequenceKind::goto_repeat) {
        auto token = value.kind == SequenceKind::consecutive_repeat ? "*" :
                     value.kind == SequenceKind::nonconsecutive_repeat ? "=" : "->";
        auto maximum = value.maximum == Sequence::unbounded ? "$" :
            std::to_string(value.maximum);
        return sequence(*value.left) + "[" + token +
               std::to_string(value.minimum) + ":" + maximum + "]";
    }
    if (value.kind == SequenceKind::match ||
        value.kind == SequenceKind::first_match) {
        std::ostringstream output;
        if (value.kind == SequenceKind::first_match) output << "first_match(";
        else output << '(';
        output << sequence(*value.left);
        for (const auto& item : value.match_items)
            output << ", " << expression(item);
        output << ')';
        return output.str();
    }
    auto name = value.kind == SequenceKind::conjunction ? "and" :
                value.kind == SequenceKind::disjunction ? "or" :
                value.kind == SequenceKind::intersection ? "intersect" :
                value.kind == SequenceKind::throughout ? "throughout" :
                value.kind == SequenceKind::within ? "within" : "sequence";
    if (value.left && value.right)
        return std::string(name) + "(" + sequence(*value.left) + ", " +
               sequence(*value.right) + ")";
    if (value.left) return std::string(name) + "(" + sequence(*value.left) + ")";
    return std::string(name);
}

std::string formula(const Property& value) {
    if (value.kind == PropertyKind::sequence)
        return value.sequence ? sequence(*value.sequence) : "<sequence>";
    if (value.kind == PropertyKind::negation)
        return "!(" + formula(*value.left) + ")";
    if (value.kind == PropertyKind::always)
        return "G(" + formula(*value.left) + ")";
    if (value.kind == PropertyKind::eventually)
        return "F(" + formula(*value.left) + ")";
    if (value.kind == PropertyKind::nexttime)
        return "X(" + formula(*value.left) + ")";
    if (value.kind == PropertyKind::until)
        return "(" + formula(*value.left) +
               (value.strong ? " U " : " W ") + formula(*value.right) + ")";
    if (value.kind == PropertyKind::conjunction ||
        value.kind == PropertyKind::disjunction ||
        value.kind == PropertyKind::implication ||
        value.kind == PropertyKind::iff) {
        auto operation = value.kind == PropertyKind::conjunction ? "&&" :
                         value.kind == PropertyKind::disjunction ? "||" :
                         value.kind == PropertyKind::implication ? "->" : "<->";
        return "(" + formula(*value.left) + " " + operation + " " +
               formula(*value.right) + ")";
    }
    if (value.left) return formula(*value.left);
    return "<property>";
}

std::string_view directive_kind(frontend::sva::DirectiveKind kind) {
    using frontend::sva::DirectiveKind;
    switch (kind) {
    case DirectiveKind::assert_property: return "assert";
    case DirectiveKind::assume_property: return "assume";
    case DirectiveKind::restrict_property: return "restrict";
    case DirectiveKind::cover_property: return "cover";
    }
    return {};
}
}

void print(const Module& module, std::ostream& output) {
    output << "btor2.hir {\n";
    for (const auto& operation : module.operations) {
        output << "  " << directive_kind(operation.directive.kind) << ' '
               << operation.directive.name << " {\n";
        for (const auto& component : operation.components)
            output << "    " << spelling(component.opcode) << " = "
                   << formula(component.formula) << '\n';
        output << "  }\n";
    }
    output << "}\n";
}
} 
