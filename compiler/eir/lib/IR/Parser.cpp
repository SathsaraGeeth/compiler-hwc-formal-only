/*
 * compiler/eir/lib/IR/Parser.cpp
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

#include "../../include/IR/Parser.h"
#include <cctype>
#include <istream>
#include <stdexcept>

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

Value declaration(std::string text, std::string fallback = {}) {
    auto colon = text.find(':');
    if (colon == std::string::npos) return {std::move(fallback), trim(std::move(text))};
    return {trim(text.substr(0, colon)), trim(text.substr(colon + 1))};
}

[[noreturn]] void fail(size_t line, const std::string& message) {
    throw std::runtime_error("EIR line " + std::to_string(line) + ": " + message);
}
}

Program parse(std::istream& input) {
    Program program;
    Module* module = nullptr;
    std::string line;
    size_t line_number = 0;
    bool has_version = false;
    while (std::getline(input, line)) {
        ++line_number;
        line = trim(std::move(line));
        if (line.empty() || line.starts_with("#") || line.starts_with("//")) continue;
        if (!has_version) {
            if (line != "eir.version 1") fail(line_number, "expected eir.version 1");
            has_version = true;
            continue;
        }
        if (line.starts_with("module ")) {
            if (module) fail(line_number, "nested module");
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
                fail(line_number, "malformed module header");
            program.modules.push_back({.name = trim(line.substr(at + 1, open - at - 1))});
            module = &program.modules.back();
            for (auto& item : split(line.substr(open + 1, close - open - 1)))
                module->inputs.push_back(declaration(item));
            auto arrow = line.find("->", close);
            if (arrow != line.npos) {
                auto result_open = line.find('(', arrow);
                auto result_close = line.find(')', result_open);
                if (result_open == line.npos || result_close == line.npos)
                    fail(line_number, "malformed module results");
                size_t index = 0;
                for (auto& item : split(
                         line.substr(result_open + 1,
                                     result_close - result_open - 1)))
                    module->results.push_back(
                        declaration(item, "result" + std::to_string(index++)));
            }
            continue;
        }
        if (line == "endmodule") {
            if (!module)
                fail(line_number, "endmodule without module");
            module = nullptr;
            continue;
        }
        if (line == "}") continue;
        if (!module)
            fail(line_number, "content outside module");
        if (line.starts_with("state ")) {
            auto state = declaration(line.substr(6));
            module->states.push_back({std::move(state.name), std::move(state.type)});
            continue;
        }
        Operation operation;
        auto equal = line.find(" = ");
        auto instruction = line;
        if (equal != line.npos) {
            auto definition = line.substr(0, equal);
            if (definition.find(':') == std::string::npos)
                operation.result = trim(std::move(definition));
            else {
                auto value = declaration(std::move(definition));
                operation.result = std::move(value.name);
                operation.result_type = std::move(value.type);
            }
            instruction = line.substr(equal + 3);
        }
        auto space = instruction.find(' ');
        operation.opcode = instruction.substr(0, space);
        operation.operands = space == instruction.npos ? "" : trim(instruction.substr(space + 1));
        module->operations.push_back(std::move(operation));
    }
    if (!has_version)
        fail(1, "missing EIR version");
    if (module)
        fail(line_number, "unterminated module");
    return program;
}
}
