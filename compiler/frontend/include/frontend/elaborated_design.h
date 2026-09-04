/*
 * compiler/frontend/include/frontend/elaborated_design.h
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

#include "frontend/semantic_tree.h"
#include "frontend/SVA/Model/directive.h"
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace emul::frontend {
class Frontend;
class SemanticTree;

class HardwareDesign {
public:
    
    const std::vector<SemanticNode>& instances() const { return instances_; }

private:
    friend class ElaboratedDesign;
    std::vector<SemanticNode> instances_;
};

class HostDesign {
public:
    
    const std::vector<SemanticNode>& instances() const { return instances_; }

    
    SemanticNode root() const { return root_; }

private:
    friend class ElaboratedDesign;
    std::vector<SemanticNode> instances_;
    SemanticNode root_;
};

struct DesignBinding {
    
    SemanticNode host;

    
    SemanticNode hardware;
};

class Bindings {
public:
    
    const std::vector<DesignBinding>& entries() const { return entries_; }

private:
    friend class ElaboratedDesign;
    std::vector<DesignBinding> entries_;
};

struct FormalModule {
    std::string module;
    std::vector<sva::Directive> directives;
};

class FormalDesign {
public:
    const std::vector<FormalModule>& modules() const { return modules_; }
    const sva::Directive& directive(std::string_view module,
                                    std::string_view name) const;
    std::vector<sva::Directive> environment(std::string_view module) const;

private:
    friend class ElaboratedDesign;
    std::vector<FormalModule> modules_;
};

class ElaboratedDesign {
public:
    
    ElaboratedDesign() = default;

    
    explicit operator bool() const;

    
    SemanticNode root() const;

    
    SemanticNode detailed_root() const;

    
    const HardwareDesign& hardware_design() const { return hardware_; }

    
    const HostDesign& host_design() const { return host_; }

    
    const Bindings& bindings() const { return bindings_; }

    
    const FormalDesign& formal_design() const { return formal_; }

    
    void print(std::ostream& output) const;

private:
    friend class Frontend;

    
    explicit ElaboratedDesign(std::shared_ptr<SemanticTree> tree);

    
    std::shared_ptr<SemanticTree> tree_;

    HardwareDesign hardware_;
    HostDesign host_;
    Bindings bindings_;
    FormalDesign formal_;
};
}  
