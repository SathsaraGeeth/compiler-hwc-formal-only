/*
 * compiler/eir/lib/IR/Operation.cpp
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

#include "../../include/IR/Operation.h"
#include <cctype>

namespace emul::eir {
namespace {

std::string trim(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.erase(text.begin());
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.pop_back();
    return text;
}

std::vector<std::string> split(std::string_view source) {
    std::vector<std::string> result;
    size_t start = 0;
    int depth = 0;
    for (size_t index = 0; index <= source.size(); ++index) {
        if (index < source.size()) {
            depth += source[index] == '(' ? 1 : source[index] == ')' ? -1 : 0;
            if (source[index] != ',' || depth) continue;
        }
        auto item = trim(std::string(source.substr(start, index - start)));
        if (!item.empty()) result.push_back(std::move(item));
        start = index + 1;
    }
    return result;
}
}

std::vector<std::string> Operation::operand_list() const {
    return split(operands);
}

std::vector<std::string> Operation::result_list() const {
    return split(result);
}

bool Operation::has_result() const noexcept {
    return !result.empty();
}

bool Operation::is_terminator() const noexcept {
    return opcode == "yield";
}

bool Operation::has_side_effects() const noexcept {
    return opcode == "state_write" || opcode == "export_write" || opcode == "yield";
}

Type Operation::type() const {
    return Type::parse(result_type);
}
}
