/*
 * compiler/eir/lib/Lowering/Frontend/context.h
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

#pragma once
#include "../../../include/IR/Module.h"
#include "frontend/semantic_tree.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <optional>

namespace emul::lowering::semantic {
class UnresolvedValue : public std::runtime_error {
public:
    explicit UnresolvedValue(const std::string& name)
        : std::runtime_error("unresolved combinational value: " + name),
          name_(name) {}

    const std::string& name() const { return name_; }

private:
    std::string name_;
};

struct LoweredValue { std::string name; std::string type; };

class Context {
public:
    explicit Context(eir::Module& module) : module(module) {}
    LoweredValue expression(frontend::SemanticNode node);
    void statement(frontend::SemanticNode node, const LoweredValue* enable,
                   bool sequential);
    void assign(frontend::SemanticNode target, LoweredValue value,
                const LoweredValue* enable, bool sequential);
    LoweredValue emit(std::string opcode, std::string operands,
                      std::string type = {});
    LoweredValue read(std::string_view name, std::string type);
    std::string temporary();

    eir::Module& module;
    std::unordered_map<std::string, LoweredValue> values;
    std::unordered_map<std::string, std::string> types;
    std::unordered_set<std::string> states;
    std::unordered_map<std::string, size_t> memories;
    std::unordered_map<std::string, LoweredValue> pending;
    std::optional<LoweredValue> return_value;
    std::optional<LoweredValue> lvalue_reference;
    size_t next_value = 0;
};

std::string symbol_name(frontend::SemanticNode node);
void register_type_aliases(frontend::SemanticNode root);
frontend::SemanticNode find_subroutine(std::string_view reference);
std::string lower_type(std::string_view slang_type);
std::string lower_type(frontend::SemanticNode node);
unsigned type_width(std::string_view type);
unsigned member_offset(std::string_view aggregate_type, std::string_view member);
}
