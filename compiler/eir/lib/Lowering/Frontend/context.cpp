/*
 * compiler/eir/lib/Lowering/Frontend/context.cpp
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

#include "context.h"
#include <regex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace emul::lowering::semantic {
namespace {
std::unordered_map<std::string, std::string> aliases;
std::unordered_map<std::string, frontend::SemanticNode> subroutines;

std::string unqualified(std::string value) {
    auto space = value.rfind(' ');
    if (space != value.npos) value = value.substr(space + 1);
    auto package = value.rfind("::");
    return package == value.npos ? value : value.substr(package + 2);
}

std::string alias_target(std::string_view type) {
    auto space = type.find(' ');
    if (space != type.npos) {
        auto found = aliases.find(std::string(type.substr(0, space)));
        if (found != aliases.end()) return found->second;
    }
    auto name = unqualified(std::string(type));
    auto dollar = name.find('$');
    if (dollar != name.npos) name.resize(dollar);
    auto found = aliases.find(name);
    return found == aliases.end() ? std::string{} : found->second;
}

std::vector<std::pair<std::string, unsigned>> packed_members(std::string_view type) {
    auto resolved = alias_target(type);
    if (!resolved.empty()) type = resolved;
    auto begin = type.find('{'), end = type.rfind('}');
    if (begin == type.npos || end == type.npos || end <= begin) return {};
    std::vector<std::pair<std::string, unsigned>> result;
    std::string body(type.substr(begin + 1, end - begin - 1));
    size_t start = 0;
    while (start < body.size()) {
        auto semi = body.find(';', start);
        if (semi == body.npos) break;
        auto declaration = body.substr(start, semi - start);
        auto split = declaration.find_last_of(" \t");
        if (split != declaration.npos) {
            auto field_type = declaration.substr(0, split);
            auto name = declaration.substr(split + 1);
            while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) name.erase(0, 1);
            if (!name.empty()) result.emplace_back(name, type_width(lower_type(field_type)));
        }
        start = semi + 1;
    }
    return result;
}

bool packed_is_four_state(std::string_view type) {
    auto resolved = alias_target(type);
    if (!resolved.empty()) type = resolved;
    auto begin = type.find('{'), end = type.rfind('}');
    if (begin == type.npos || end == type.npos || end <= begin) return false;
    std::string body(type.substr(begin + 1, end - begin - 1));
    size_t start = 0;
    while (start < body.size()) {
        auto semi = body.find(';', start);
        if (semi == body.npos) break;
        auto declaration = body.substr(start, semi - start);
        auto split = declaration.find_last_of(" \t");
        if (split != declaration.npos &&
            lower_type(declaration.substr(0, split)).starts_with("4s<"))
            return true;
        start = semi + 1;
    }
    return false;
}
}

void register_type_aliases(frontend::SemanticNode root) {
    aliases.clear();
    subroutines.clear();
    std::unordered_set<int64_t> seen;
    auto visit = [&](auto&& self, frontend::SemanticNode node) -> void {
        if (!node) return;
        auto address = node.integer("addr", 0);
        if (address && !seen.insert(address).second) return;
        if (node.kind() == "TypeAlias" || node.kind() == "EnumType") {
            auto target = node.kind() == "EnumType" ? node.text("baseType") : node.text("target");
            if (address) aliases[std::to_string(address)] = target;
            if (!node.name().empty()) aliases[node.name()] = target;
        }
        if (node.kind() == "Subroutine" && address)
            subroutines[std::to_string(address)] = node;
        for (const auto& field : node.fields()) {
            auto value = node.value(field);
            if (value.kind() == frontend::SemanticValueKind::object)
                self(self, node.child(field));
            else if (value.kind() == frontend::SemanticValueKind::array)
                for (auto child : node.children(field)) self(self, child);
        }
    };
    visit(visit, root);
}

frontend::SemanticNode find_subroutine(std::string_view reference) {
    auto space = reference.find(' ');
    auto key = std::string(reference.substr(0, space));
    auto found = subroutines.find(key);
    return found == subroutines.end() ? frontend::SemanticNode{} : found->second;
}

std::string symbol_name(frontend::SemanticNode node) {
    auto symbol = node.text("symbol");
    auto space = symbol.rfind(' ');
    if (node.kind() == "HierarchicalValue") {
        auto address = space == std::string::npos ? std::string{} : symbol.substr(0, space);
        auto name = space == std::string::npos ? symbol : symbol.substr(space + 1);
        return "__hier_" + address + "_" + name;
    }
    return space == std::string::npos ? symbol : symbol.substr(space + 1);
}

std::string lower_type(std::string_view source) {
    auto resolved = alias_target(source);
    if (!resolved.empty() && resolved != source) return lower_type(resolved);
    if (source.find("struct packed{") != source.npos) {
        unsigned width = 0;
        for (const auto& [name, field_width] : packed_members(source)) {
            (void)name;
            width += field_width;
        }
        return std::string(packed_is_four_state(source) ? "4s<" : "2s<") +
               std::to_string(width) + ">";
    }
    bool four_state = source.starts_with("logic") || source.starts_with("integer") ||
                      source.starts_with("time");
    std::cmatch match;
    static const std::regex range(R"(\[([0-9]+):([0-9]+)\])");
    unsigned width = 1;
    if (std::regex_search(source.begin(), source.end(), match, range)) {
        auto left = std::stoul(match[1].str()), right = std::stoul(match[2].str());
        width = left > right ? left - right + 1 : right - left + 1;
    } else if (source.starts_with("int") || source.starts_with("integer")) {
        width = 32;
    } else if (source.starts_with("longint") || source.starts_with("time")) {
        width = 64;
    }
    return std::string(four_state ? "4s<" : "2s<") + std::to_string(width) + ">";
}

std::string lower_type(frontend::SemanticNode node) {
    auto resolved = node.child("type");
    while (resolved && resolved.kind() == "TypeAlias")
        resolved = resolved.child("target");
    if (resolved && resolved.kind() == "EnumType")
        return lower_type(resolved.text("baseType"));
    return lower_type(node.type());
}

unsigned type_width(std::string_view type) {
    auto begin = type.find('<'), end = type.find('>');
    if (begin == type.npos || end == type.npos) throw std::runtime_error("invalid EIR type");
    return std::stoul(std::string(type.substr(begin + 1, end - begin - 1)));
}

unsigned member_offset(std::string_view aggregate_type, std::string_view member) {
    auto fields = packed_members(aggregate_type);
    unsigned offset = 0;
    for (auto field = fields.rbegin(); field != fields.rend(); ++field) {
        if (field->first == member) return offset;
        offset += field->second;
    }
    throw std::runtime_error("unknown packed member: " + std::string(member));
}

std::string Context::temporary() { return "%" + std::to_string(next_value++); }
LoweredValue Context::emit(std::string opcode, std::string operands, std::string type) {
    LoweredValue result{temporary(), std::move(type)};
    module.operations.push_back({result.name, result.type, std::move(opcode), std::move(operands)});
    return result;
}
LoweredValue Context::read(std::string_view name, std::string type) {
    if (states.contains(std::string(name)))
        return emit("state_read", "@" + std::string(name), type);
    auto found = values.find(std::string(name));
    if (found != values.end()) return found->second;
    if (types.contains(std::string(name))) throw UnresolvedValue(std::string(name));
    return {"%" + std::string(name), std::move(type)};
}
}
