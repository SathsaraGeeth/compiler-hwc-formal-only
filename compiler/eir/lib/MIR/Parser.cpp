/*
 * compiler/eir/lib/MIR/Parser.cpp
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

#include "../../include/MIR/Parser.h"
#include <cctype>
#include <istream>
#include <stdexcept>

namespace emul::mir {
namespace {
std::string trim(std::string text) {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.front())))
        text.erase(text.begin());
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back())))
        text.pop_back();
    return text;
}
std::vector<std::string> split(std::string_view source) {
    std::vector<std::string> result;
    size_t start = 0;
    int depth = 0;
    for (size_t index = 0; index <= source.size(); ++index) {
        if (index < source.size()) {
            depth += source[index] == '(' || source[index] == '<' ? 1 :
                     source[index] == ')' || source[index] == '>' ? -1 : 0;
            if (source[index] != ',' || depth) continue;
        }
        auto item = trim(std::string(source.substr(start, index - start)));
        if (!item.empty()) result.push_back(std::move(item));
        start = index + 1;
    }
    return result;
}
eir::Value declaration(std::string text) {
    auto colon = text.find(':');
    if (colon == std::string::npos)
        throw std::runtime_error("MIR declaration requires a type");
    return {trim(text.substr(0, colon)), trim(text.substr(colon + 1))};
}
[[noreturn]] void fail(size_t line, const std::string& message) {
    throw std::runtime_error("MIR line " + std::to_string(line) + ": " + message);
}
}

MachineModule parse(std::istream& input) {
    MachineModule result;
    MachineFunction* function = nullptr;
    MachineBlock* block = nullptr;
    std::string line;
    size_t line_number = 0;
    bool version = false;
    while (std::getline(input, line)) {
        ++line_number;
        line = trim(std::move(line));
        if (line.empty() || line.starts_with("#") || line.starts_with("//")) continue;
        if (!version) {
            if (line != "mir.version 1") fail(line_number, "expected mir.version 1");
            version = true;
            continue;
        }
        if (line.starts_with("machine ")) {
            if (function) fail(line_number, "nested machine");
            while (!line.ends_with("{") && input) {
                std::string continuation;
                if (!std::getline(input, continuation)) break;
                ++line_number;
                line += " " + trim(std::move(continuation));
            }
            auto at = line.find('@');
            auto open = line.find('(', at);
            auto close = line.find(')', open);
            if (at == line.npos || open == line.npos ||
                close == line.npos || line.rfind('{') == line.npos)
                fail(line_number, "malformed machine header");
            result.functions.push_back({
                .name = trim(line.substr(at + 1, open - at - 1))
            });
            function = &result.functions.back();
            for (auto& item : split(line.substr(open + 1, close - open - 1)))
                function->inputs.push_back(declaration(item));
            auto arrow = line.find("->", close);
            if (arrow != line.npos) {
                auto result_open = line.find('(', arrow);
                auto result_close = line.find(')', result_open);
                if (result_open == line.npos || result_close == line.npos)
                    fail(line_number, "malformed machine results");
                auto results = line.substr(
                    result_open + 1,
                    result_close - result_open - 1);
                for (auto& item : split(results))
                    function->results.push_back(declaration(item));
            }
            continue;
        }
        if (line == "endmachine") {
            if (!function) fail(line_number, "endmachine without machine");
            function = nullptr;
            block = nullptr;
            continue;
        }
        if (line == "}") continue;
        if (!function) fail(line_number, "content outside machine");
        if (line.starts_with("state ")) {
            auto state = declaration(line.substr(6));
            function->states.push_back({std::move(state.name), std::move(state.type)});
            continue;
        }
        if (line.starts_with("^") && line.ends_with(":")) {
            function->blocks.push_back({.name = line.substr(1, line.size() - 2)});
            block = &function->blocks.back();
            continue;
        }
        if (!block) fail(line_number, "instruction outside block");
        MachineInstr instruction;
        auto equal = line.find(" = ");
        auto body = line;
        if (equal != line.npos) {
            auto definition = declaration(line.substr(0, equal));
            instruction.result = std::move(definition.name);
            instruction.result_type = std::move(definition.type);
            body = line.substr(equal + 3);
        }
        auto space = body.find(' ');
        auto opcode = parse_opcode(body.substr(0, space));
        if (!opcode) fail(line_number, "unknown opcode " + body.substr(0, space));
        instruction.opcode = *opcode;
        if (space != body.npos)
            for (auto& operand : split(body.substr(space + 1)))
                instruction.operands.push_back(
                    MachineOperand::classify(std::move(operand)));
        block->instructions.push_back(std::move(instruction));
    }
    if (!version) fail(1, "missing MIR version");
    if (function) fail(line_number, "unterminated machine");
    return result;
}
}
