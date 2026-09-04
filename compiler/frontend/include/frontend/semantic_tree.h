/*
 * compiler/frontend/include/frontend/semantic_tree.h
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
#include <iosfwd>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct json_object;

namespace emul::frontend {

enum class SemanticValueKind {
    null,
    boolean,
    integer,
    real,
    string,
    object,
    array
};

class SemanticValue {
public:
    
    SemanticValue() = default;

    
    explicit operator bool() const { return present_; }

    
    SemanticValueKind kind() const;

    
    bool boolean(bool fallback = false) const;

    
    int64_t integer(int64_t fallback = 0) const;

    
    double real(double fallback = 0.0) const;

    
    std::string string() const;

    
    std::vector<std::string> fields() const;

    
    SemanticValue field(std::string_view name) const;

    
    std::vector<SemanticValue> elements() const;

private:
    friend class SemanticNode;
    SemanticValue(std::shared_ptr<json_object> owner, json_object* value, bool present = true)
        : owner_(std::move(owner)), value_(value), present_(present) {}
    std::shared_ptr<json_object> owner_;
    json_object* value_ = nullptr;
    bool present_ = false;
};

class SemanticNode {
public:
    
    SemanticNode() = default;

    
    explicit operator bool() const { return node_ != nullptr; }

    
    std::string kind() const;

    
    std::string name() const;

    
    std::string type() const;

    
    std::string text(std::string_view field) const;

    
    bool boolean(std::string_view field, bool fallback = false) const;

    
    int64_t integer(std::string_view field, int64_t fallback = 0) const;

    
    double real(std::string_view field, double fallback = 0.0) const;

    
    std::vector<std::string> fields() const;

    
    SemanticValue value(std::string_view field) const;

    
    SemanticNode child(std::string_view field) const;

    
    std::vector<SemanticNode> children(std::string_view field) const;
private:
    friend class SemanticTree;
    SemanticNode(std::shared_ptr<json_object> owner, json_object* node, json_object* graph)
        : owner_(std::move(owner)), node_(node), graph_(graph) {}
    std::shared_ptr<json_object> owner_;
    json_object* node_ = nullptr;
    json_object* graph_ = nullptr;
};

class SemanticTree {
public:
    
    static std::shared_ptr<SemanticTree> parse(std::string_view serialized);

    
    SemanticNode root() const;

    
    SemanticNode detailed_root() const;

    
    void print(std::ostream& output) const;
private:
    explicit SemanticTree(std::shared_ptr<json_object> root) : root_(std::move(root)) {}
    std::shared_ptr<json_object> root_;
};
} 
