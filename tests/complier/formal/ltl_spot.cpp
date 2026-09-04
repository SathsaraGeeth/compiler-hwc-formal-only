#include "Lowering/Formal/LTL/spot_automaton.h"
#include "Lowering/Formal/LTL/sequence_formula.h"
#include <memory>
#include <stdexcept>

using emul::formal::ltl::SpotAutomaton;
using emul::frontend::sva::Property;
using emul::frontend::sva::PropertyKind;
using emul::frontend::sva::Sequence;
using emul::frontend::sva::SequenceKind;

namespace {
std::shared_ptr<Property> atom(const char* name) {
    auto value = std::make_shared<Property>();
    value->kind = PropertyKind::sequence;
    value->sequence = std::make_shared<Sequence>();
    value->sequence->kind = SequenceKind::atom;
    value->sequence->expression.value = name;
    return value;
}

std::shared_ptr<Property> delayed(
    const char* left,
    uint32_t minimum,
    uint32_t maximum,
    const char* right) {
    auto value = std::make_shared<Property>();
    value->kind = PropertyKind::sequence;
    value->sequence = std::make_shared<Sequence>();
    value->sequence->kind = SequenceKind::concatenation;
    value->sequence->minimum = minimum;
    value->sequence->maximum = maximum;
    value->sequence->left = atom(left)->sequence;
    value->sequence->right = atom(right)->sequence;
    return value;
}

std::shared_ptr<Property> sequence_or(
    std::shared_ptr<Property> left,
    std::shared_ptr<Property> right) {
    auto value = std::make_shared<Property>();
    value->kind = PropertyKind::sequence;
    value->sequence = std::make_shared<Sequence>();
    value->sequence->kind = SequenceKind::disjunction;
    value->sequence->left = std::move(left->sequence);
    value->sequence->right = std::move(right->sequence);
    return value;
}

std::shared_ptr<Property> sequence_binary(
    SequenceKind kind,
    std::shared_ptr<Property> left,
    std::shared_ptr<Property> right) {
    auto value = std::make_shared<Property>();
    value->kind = PropertyKind::sequence;
    value->sequence = std::make_shared<Sequence>();
    value->sequence->kind = kind;
    value->sequence->left = std::move(left->sequence);
    value->sequence->right = std::move(right->sequence);
    return value;
}

std::shared_ptr<Property> repeated(
    const char* name, uint32_t minimum, uint32_t maximum,
    SequenceKind kind = SequenceKind::consecutive_repeat) {
    auto value = atom(name);
    auto operand = value->sequence;
    value->sequence = std::make_shared<Sequence>();
    value->sequence->kind = kind;
    value->sequence->minimum = minimum;
    value->sequence->maximum = maximum;
    value->sequence->left = std::move(operand);
    return value;
}

std::shared_ptr<Property> unary(
    PropertyKind kind,
    std::shared_ptr<Property> operand) {
    auto value = std::make_shared<Property>();
    value->kind = kind;
    value->left = std::move(operand);
    value->maximum = UINT32_MAX;
    return value;
}

std::shared_ptr<Property> binary(
    PropertyKind kind,
    std::shared_ptr<Property> left,
    std::shared_ptr<Property> right) {
    auto value = std::make_shared<Property>();
    value->kind = kind;
    value->left = std::move(left);
    value->right = std::move(right);
    return value;
}

std::shared_ptr<Property> conditional(
    const char* condition,
    std::shared_ptr<Property> when_true,
    std::shared_ptr<Property> when_false = {}) {
    auto value = std::make_shared<Property>();
    value->kind = PropertyKind::conditional;
    value->condition.value = condition;
    value->left = std::move(when_true);
    value->right = std::move(when_false);
    return value;
}

bool valid(const emul::formal::ltl::Automaton& value) {
    return value.states && value.initial < value.states &&
           !value.transitions.empty() &&
           value.accepting.size() == value.states;
}

bool accepted(SpotAutomaton& translator, const Property& property) {
    try {
        (void)translator.translate(property, false);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}
}

int main() {
    SpotAutomaton translator;

    auto always_eventually = unary(PropertyKind::always,
        unary(PropertyKind::eventually, atom("request")));
    if (!valid(translator.translate(*always_eventually, true))) return 1;

    auto eventually_always = unary(PropertyKind::eventually,
        unary(PropertyKind::always, atom("grant")));
    if (!valid(translator.translate(*eventually_always, false))) return 2;

    auto until = binary(PropertyKind::until, atom("busy"), atom("done"));
    until->strong = true;
    if (!valid(translator.translate(*until, true))) return 3;

    auto next = unary(PropertyKind::nexttime, atom("response"));
    next->minimum = 3;
    next->maximum = 3;
    if (!valid(translator.translate(*next, false))) return 4;

    auto fairness = binary(PropertyKind::implication,
        std::move(eventually_always), std::move(always_eventually));
    if (!valid(translator.translate(*fairness, true))) return 5;

    auto conditional_liveness = conditional("enabled",
        unary(PropertyKind::always,
            unary(PropertyKind::eventually, atom("response"))),
        binary(PropertyKind::until, atom("idle"), atom("enabled")));
    conditional_liveness->right->strong = true;
    if (!valid(translator.translate(*conditional_liveness, true))) return 6;

    auto conditional_without_else = conditional("enabled",
        unary(PropertyKind::nexttime, atom("response")));
    if (!valid(translator.translate(*conditional_without_else, false))) return 7;

    auto bounded_sequence = unary(PropertyKind::always,
        delayed("request", 1, 4, "response"));
    if (!valid(translator.translate(*bounded_sequence, true))) return 8;

    auto exact_next_sequence = unary(PropertyKind::eventually,
        delayed("request", 1, 1, "response"));
    if (!valid(translator.translate(*exact_next_sequence, false))) return 9;

    auto alternative_sequence = unary(PropertyKind::always,
        sequence_or(delayed("read", 1, 2, "response"),
                    delayed("write", 2, 3, "response")));
    if (!valid(translator.translate(*alternative_sequence, true))) return 10;

    auto nested_sequence = delayed("request", 1, 1, "grant");
    auto tail = delayed("unused", 2, 2, "response");
    tail->sequence->left = nested_sequence->sequence;
    auto nested_temporal = unary(PropertyKind::always, std::move(tail));
    if (!valid(translator.translate(*nested_temporal, true))) return 11;

    auto repetition = unary(PropertyKind::always, repeated("valid", 2, 4));
    if (!valid(translator.translate(*repetition, true))) return 12;

    auto conjunction = sequence_binary(SequenceKind::conjunction,
        delayed("request", 1, 2, "grant"),
        delayed("enabled", 2, 3, "response"));
    if (!valid(translator.translate(*conjunction, false))) return 13;

    auto intersection = sequence_binary(SequenceKind::intersection,
        delayed("request", 1, 2, "grant"),
        delayed("enabled", 2, 3, "response"));
    if (!valid(translator.translate(*intersection, false))) return 14;

    auto throughout = sequence_binary(SequenceKind::throughout,
        atom("enabled"), delayed("request", 1, 3, "response"));
    if (!valid(translator.translate(*throughout, true))) return 15;

    auto within = sequence_binary(SequenceKind::within,
        delayed("request", 1, 1, "grant"),
        delayed("enabled", 3, 3, "response"));
    if (!valid(translator.translate(*within, true))) return 16;

    auto unbounded_delay = delayed(
        "request", 1, Sequence::unbounded, "response");
    if (!valid(translator.translate(*unbounded_delay, false))) return 17;

    auto goto_repetition = atom("request");
    goto_repetition->sequence->kind = SequenceKind::goto_repeat;
    goto_repetition->sequence->left = atom("request")->sequence;
    goto_repetition->sequence->minimum = 1;
    goto_repetition->sequence->maximum = 2;
    if (!valid(translator.translate(*goto_repetition, false))) return 18;

    auto nonconsecutive = unary(PropertyKind::always,
        repeated("request", 1, 3, SequenceKind::nonconsecutive_repeat));
    if (!accepted(translator, *nonconsecutive)) return 19;

    auto zero_repeat = unary(PropertyKind::eventually,
        repeated("idle", 0, 2));
    if (!accepted(translator, *zero_repeat)) return 20;

    auto unbounded_repeat = unary(PropertyKind::always,
        repeated("valid", 1, Sequence::unbounded));
    if (!accepted(translator, *unbounded_repeat)) return 21;

    auto first = delayed("request", 1, 8, "grant");
    auto first_operand = first->sequence;
    first->sequence = std::make_shared<Sequence>();
    first->sequence->kind = SequenceKind::first_match;
    first->sequence->left = std::move(first_operand);
    if (!accepted(translator, *first)) return 22;

    auto large_delay = delayed("request", 1, 255, "response");
    try {
        (void)emul::formal::ltl::SequenceFormula([](const auto& expression) {
            return spot::formula::ap(expression.value);
        }).lower_sere(*large_delay->sequence);
    } catch (const std::exception&) {
        return 23;
    }

    auto composite_throughout = sequence_binary(SequenceKind::throughout,
        sequence_or(atom("enabled"), atom("override")),
        delayed("request", 1, 3, "response"));
    if (!accepted(translator, *composite_throughout)) return 24;

    return 0;
}
