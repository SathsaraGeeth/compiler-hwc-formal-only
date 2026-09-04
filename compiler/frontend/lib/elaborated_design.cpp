/*
 * compiler/frontend/lib/elaborated_design.cpp
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

#include "frontend/elaborated_design.h"
#include "frontend/semantic_tree.h"
#include "frontend/SVA/Lowering/directive.h"
#include <json-c/json.h>
#include <ostream>
#include <stdexcept>
#include <unordered_set>

namespace emul::frontend {
namespace {
bool is_runtime_marker(SemanticNode instance) {
    return instance.child("body").name().starts_with("emul_runtime_");
}

bool is_host_instance(SemanticNode instance) {
    bool has_port = false;
    bool has_initial = false;
    bool has_synthesizable_process = false;
    for (auto member : instance.child("body").children("members")) {
        if (member.kind() == "Property")
            return true;
        if (member.kind() == "Port") has_port = true;
        if (member.kind() != "ProceduralBlock") continue;
        auto procedure = member.text("procedureKind");
        has_initial |= procedure == "Initial";
        has_synthesizable_process |= procedure == "AlwaysFF" ||
                                     procedure == "AlwaysComb";
    }
    
    
    
    return has_initial && !(has_port && has_synthesizable_process);
}

std::string normalized_name(std::string_view value) {
    const auto terminator = value.find('\0');
    return std::string(value.substr(0, terminator));
}

void print_string(std::ostream& output, const std::string& value) {
    auto* string = json_object_new_string(value.c_str());
    output << json_object_to_json_string_ext(string, JSON_C_TO_STRING_PLAIN);
    json_object_put(string);
}

std::string_view directive_kind(sva::DirectiveKind kind) {
    switch (kind) {
    case sva::DirectiveKind::assert_property: return "assert";
    case sva::DirectiveKind::assume_property: return "assume";
    case sva::DirectiveKind::cover_property: return "cover";
    case sva::DirectiveKind::restrict_property: return "restrict";
    }
    return "unknown";
}
}

const sva::Directive& FormalDesign::directive(
    std::string_view module, std::string_view name) const {
    const auto normalized_module = normalized_name(module);
    const auto normalized_property = normalized_name(name);
    for (const auto& entry : modules_)
        if (entry.module == normalized_module)
            for (const auto& directive : entry.directives)
                if (directive.name == normalized_property) return directive;
    
    
    
    const sva::Directive* unique = nullptr;
    const sva::Directive* only_directive = nullptr;
    size_t directive_count = 0;
    for (const auto& entry : modules_)
        for (const auto& directive : entry.directives) {
            ++directive_count;
            only_directive = &directive;
            if (directive.name == normalized_property) {
                if (unique)
                    throw std::runtime_error("ambiguous formal property: " +
                                             normalized_property);
                unique = &directive;
            }
        }
    if (unique) return *unique;
    
    
    if (directive_count == 1) return *only_directive;
    throw std::runtime_error("formal property not found: " +
                             std::string(module) + "." + std::string(name));
}

std::vector<sva::Directive> FormalDesign::environment(
    std::string_view module) const {
    std::vector<sva::Directive> result;
    for (const auto& entry : modules_)
        if (entry.module == module)
            for (const auto& directive : entry.directives)
                if (directive.kind == sva::DirectiveKind::assume_property ||
                    directive.kind == sva::DirectiveKind::restrict_property)
                    result.push_back(directive);
    return result;
}

ElaboratedDesign::ElaboratedDesign(std::shared_ptr<SemanticTree> tree)
    : tree_(std::move(tree)) {
    host_.root_ = tree_->root();
    std::unordered_set<std::string> hardware_modules;
    std::unordered_set<std::string> formal_modules;
    auto visit = [&](auto&& self, SemanticNode instance, SemanticNode host) -> void {
        if (is_runtime_marker(instance)) return;
        auto instance_is_host = is_host_instance(instance);
        if (instance_is_host) {
            host_.instances_.push_back(instance);
            host = instance;
        } else {
            if (host) {
                bindings_.entries_.push_back({host, instance});
                host = {};
            }
        }
        for (auto member : instance.child("body").children("members"))
            if (member.kind() == "Instance") self(self, member, host);
        if (!instance_is_host) {
            auto module = instance.child("body").name();
            if (hardware_modules.insert(module).second)
                hardware_.instances_.push_back(instance);
        }
        auto module = normalized_name(instance.child("body").name());
        if (formal_modules.insert(module).second)
            formal_.modules_.push_back(
                {module, sva::lower_directives(instance.child("body"))});
    };
    for (auto member : tree_->root().children("members"))
        if (member.kind() == "Instance") visit(visit, member, {});
}

ElaboratedDesign::operator bool() const {
    return static_cast<bool>(tree_);
}

SemanticNode ElaboratedDesign::root() const {
    if (!tree_)
        throw std::runtime_error("elaborated design is unavailable");
    return tree_->root();
}

SemanticNode ElaboratedDesign::detailed_root() const {
    if (!tree_)
        throw std::runtime_error("elaborated design is unavailable");
    return tree_->detailed_root();
}

void ElaboratedDesign::print(std::ostream& output) const {
    if (!tree_)
        throw std::runtime_error("elaborated design is unavailable");
    output << "{\n  \"hardwareDesign\": {\n    \"instances\": [";
    for (size_t index = 0; index < hardware_.instances_.size(); ++index) {
        auto instance = hardware_.instances_[index];
        output << (index ? ",\n" : "\n") << "      {\"instance\": ";
        print_string(output, instance.name());
        output << ", \"module\": ";
        print_string(output, instance.child("body").name());
        output << '}';
    }
    output << "\n    ]\n  },\n  \"hostDesign\": {\n    \"instances\": [";
    for (size_t index = 0; index < host_.instances_.size(); ++index) {
        auto instance = host_.instances_[index];
        output << (index ? ",\n" : "\n") << "      {\"instance\": ";
        print_string(output, instance.name());
        output << ", \"module\": ";
        print_string(output, instance.child("body").name());
        output << '}';
    }
    output << "\n    ]\n  },\n  \"bindings\": [";
    for (size_t index = 0; index < bindings_.entries_.size(); ++index) {
        const auto& binding = bindings_.entries_[index];
        output << (index ? ",\n" : "\n") << "    {\"host\": ";
        print_string(output, binding.host.name());
        output << ", \"instance\": ";
        print_string(output, binding.hardware.name());
        output << ", \"hardwareModule\": ";
        print_string(output, binding.hardware.child("body").name());
        output << '}';
    }
    output << "\n  ],\n  \"formalDesign\": {\n    \"modules\": [";
    for (size_t index = 0; index < formal_.modules_.size(); ++index) {
        const auto& module = formal_.modules_[index];
        output << (index ? ",\n" : "\n") << "      {\"module\": ";
        print_string(output, module.module);
        output << ", \"directives\": [";
        for (size_t directive = 0; directive < module.directives.size(); ++directive) {
            const auto& value = module.directives[directive];
            output << (directive ? ", " : "") << "{\"name\": ";
            print_string(output, value.name);
            output << ", \"kind\": ";
            print_string(output, std::string(directive_kind(value.kind)));
            output << ", \"clock\": ";
            print_string(output, value.clock.signal);
            output << '}';
        }
        output << "]}";
    }
    output << "\n    ]\n  },\n  \"semanticTree\": ";
    tree_->print(output);
    output << "}\n";
}
} 
