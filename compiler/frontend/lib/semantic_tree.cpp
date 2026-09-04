/*
 * compiler/frontend/lib/semantic_tree.cpp
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

#include "frontend/semantic_tree.h"
#include <json-c/json.h>
#include <charconv>
#include <ostream>
#include <stdexcept>

namespace emul::frontend {
namespace {
json_object* field(json_object* node, std::string_view name) {
    if (!node || json_object_get_type(node) != json_type_object) return nullptr;
    json_object* result = nullptr;
    std::string key(name);
    return json_object_object_get_ex(node, key.c_str(), &result) ? result : nullptr;
}
std::string string_field(json_object* node, std::string_view name) {
    auto* value = field(node, name);
    return value && json_object_get_type(value) == json_type_string
        ? json_object_get_string(value) : std::string{};
}
json_object* find_address(json_object* node, uint64_t address) {
    if (!node) return nullptr;
    auto type = json_object_get_type(node);
    if (type == json_type_object) {
        if (auto* candidate = field(node, "addr"); candidate &&
            static_cast<uint64_t>(json_object_get_int64(candidate)) == address)
            return node;
        json_object_object_foreach(node, key, value) {
            (void)key;
            if (auto* found = find_address(value, address)) return found;
        }
    } else if (type == json_type_array) {
        auto count = json_object_array_length(node);
        for (size_t index = 0; index < count; ++index)
            if (auto* found = find_address(
                    json_object_array_get_idx(node, index), address)) {
                return found;
            }
    }
    return nullptr;
}
json_object* resolve(json_object* root, json_object* value) {
    if (!value || json_object_get_type(value) != json_type_string) return value;
    std::string_view reference = json_object_get_string(value);
    uint64_t address = 0;
    auto [end, error] = std::from_chars(
        reference.data(), reference.data() + reference.size(), address);
    if (error != std::errc{} || end == reference.data()) return value;
    return find_address(root, address);
}
}

SemanticValueKind SemanticValue::kind() const {
    if (!value_ || json_object_get_type(value_) == json_type_null)
        return SemanticValueKind::null;
    switch (json_object_get_type(value_)) {
        case json_type_boolean: return SemanticValueKind::boolean;
        case json_type_int: return SemanticValueKind::integer;
        case json_type_double: return SemanticValueKind::real;
        case json_type_string: return SemanticValueKind::string;
        case json_type_object: return SemanticValueKind::object;
        case json_type_array: return SemanticValueKind::array;
        case json_type_null: return SemanticValueKind::null;
    }
    return SemanticValueKind::null;
}

bool SemanticValue::boolean(bool fallback) const {
    return value_ && json_object_get_type(value_) == json_type_boolean
        ? json_object_get_boolean(value_) : fallback;
}

int64_t SemanticValue::integer(int64_t fallback) const {
    return value_ && json_object_get_type(value_) == json_type_int
        ? json_object_get_int64(value_) : fallback;
}

double SemanticValue::real(double fallback) const {
    return value_ && json_object_get_type(value_) == json_type_double
        ? json_object_get_double(value_) : fallback;
}

std::string SemanticValue::string() const {
    return value_ && json_object_get_type(value_) == json_type_string
        ? json_object_get_string(value_) : std::string{};
}

std::vector<std::string> SemanticValue::fields() const {
    std::vector<std::string> result;
    if (!value_ || json_object_get_type(value_) != json_type_object) return result;
    json_object_object_foreach(value_, key, value) {
        (void)value;
        result.emplace_back(key);
    }
    return result;
}

SemanticValue SemanticValue::field(std::string_view name) const {
    if (!value_ || json_object_get_type(value_) != json_type_object) return {};
    json_object* result = nullptr;
    std::string key(name);
    auto present = json_object_object_get_ex(value_, key.c_str(), &result);
    return {owner_, result, present != 0};
}

std::vector<SemanticValue> SemanticValue::elements() const {
    std::vector<SemanticValue> result;
    if (!value_ || json_object_get_type(value_) != json_type_array) return result;
    auto count = json_object_array_length(value_);
    result.reserve(count);
    for (size_t index = 0; index < count; ++index)
        result.push_back(SemanticValue(owner_, json_object_array_get_idx(value_, index)));
    return result;
}

std::string SemanticNode::kind() const { return string_field(node_, "kind"); }
std::string SemanticNode::name() const { return string_field(node_, "name"); }
std::string SemanticNode::type() const { return string_field(node_, "type"); }
std::string SemanticNode::text(std::string_view name) const { return string_field(node_, name); }
bool SemanticNode::boolean(std::string_view name, bool fallback) const {
    auto* value = field(node_, name);
    return value && json_object_get_type(value) == json_type_boolean
        ? json_object_get_boolean(value) : fallback;
}
int64_t SemanticNode::integer(std::string_view name, int64_t fallback) const {
    return value(name).integer(fallback);
}
double SemanticNode::real(std::string_view name, double fallback) const {
    return value(name).real(fallback);
}
std::vector<std::string> SemanticNode::fields() const {
    return SemanticValue(owner_, node_).fields();
}
SemanticValue SemanticNode::value(std::string_view name) const {
    if (!node_ || json_object_get_type(node_) != json_type_object) return {};
    json_object* result = nullptr;
    std::string key(name);
    auto present = json_object_object_get_ex(node_, key.c_str(), &result);
    return {owner_, result, present != 0};
}
SemanticNode SemanticNode::child(std::string_view name) const {
    return {owner_, resolve(graph_, field(node_, name)), graph_};
}
std::vector<SemanticNode> SemanticNode::children(std::string_view name) const {
    std::vector<SemanticNode> result;
    auto* array = field(node_, name);
    if (!array || json_object_get_type(array) != json_type_array) return result;
    auto count = json_object_array_length(array);
    result.reserve(count);
    for (size_t index = 0; index < count; ++index)
        result.push_back(SemanticNode(owner_,
            resolve(graph_, json_object_array_get_idx(array, index)), graph_));
    return result;
}

std::shared_ptr<SemanticTree> SemanticTree::parse(std::string_view serialized) {
    json_tokener* parser = json_tokener_new_ex(4096);
    auto* value = json_tokener_parse_ex(parser, serialized.data(), serialized.size());
    auto error = json_tokener_get_error(parser);
    json_tokener_free(parser);
    if (error != json_tokener_success || !value)
        throw std::runtime_error(std::string("cannot import linked Slang design: ") +
                                 json_tokener_error_desc(error));
    auto owner = std::shared_ptr<json_object>(value, json_object_put);
    return std::shared_ptr<SemanticTree>(new SemanticTree(std::move(owner)));
}
SemanticNode SemanticTree::root() const {
    auto* design = field(root_.get(), "design");
    return {root_, design, design};
}
SemanticNode SemanticTree::detailed_root() const {
    auto* design = field(root_.get(), "detailedDesign");
    if (!design) design = field(root_.get(), "design");
    return {root_, design, design};
}

void SemanticTree::print(std::ostream& output) const {
    output << json_object_to_json_string_ext(root_.get(), JSON_C_TO_STRING_PRETTY) << '\n';
}
} 
