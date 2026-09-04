#pragma once

#include "IRGen/IRBuilder.h"

namespace vir::irgen::frontend {
class Context {
public:
    Context(Module& module, std::string function_name,
            bool component_function = false);
    IRBuilder& builder() { return builder_; }
    BasicBlock& create_block(std::string name);
    void set_insertion_point(BasicBlock& block);
    bool terminated() const;
    void branch(BasicBlock& target);
    void finish();
    Value* self() const noexcept { return self_; }
    BasicBlock& entry() const noexcept { return function_->entry_block(); }

private:
    IRBuilder builder_;
    Function* function_ = nullptr;
    BasicBlock* block_ = nullptr;
    std::size_t next_block_ = 0;
    Value* self_ = nullptr;
};
}
