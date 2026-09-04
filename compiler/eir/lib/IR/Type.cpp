/*
 * compiler/eir/lib/IR/Type.cpp
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

#include "../../include/IR/Type.h"
#include <charconv>
#include <stdexcept>

namespace emul::eir {
namespace {

bool decode(std::string_view spelling, bool& four_state, uint32_t& width) noexcept {
    if (spelling.size() < 5 || (spelling[0] != '2' && spelling[0] != '4') ||
        spelling[1] != 's' || spelling[2] != '<' || spelling.back() != '>')
        return false;

    auto digits = spelling.substr(3, spelling.size() - 4);
    if (digits.empty())
        return false;

    uint32_t parsed = 0;
    auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), parsed);
    if (error != std::errc{} || end != digits.data() + digits.size() || parsed == 0)
        return false;

    four_state = spelling[0] == '4';
    width = parsed;
    return true;
}
}

Type Type::parse(std::string_view text) {
    Type result{std::string(text)};
    if (!result.valid())
        throw std::runtime_error("invalid EIR type " + result.spelling);
    return result;
}

Type Type::bit_vector(uint32_t width, bool four_state) {
    if (!width)
        throw std::runtime_error("EIR bit-vector width must be nonzero");
    return {std::string(four_state ? "4s<" : "2s<") + std::to_string(width) + ">"};
}

bool Type::valid() const noexcept {
    bool state = false; uint32_t bits = 0;
    return decode(spelling, state, bits);
}

bool Type::two_state() const noexcept {
    return valid() && spelling[0] == '2';
}

bool Type::four_state() const noexcept {
    return valid() && spelling[0] == '4';
}

uint32_t Type::width() const {
    bool state = false; uint32_t bits = 0;
    if (!decode(spelling, state, bits))
        throw std::runtime_error("invalid EIR type " + spelling);
    return bits;
}
}
