#include "IRGen/Frontend/Context.h"

namespace vir::irgen::frontend {
Context::Context(Module& module, std::string function_name,
                 bool component_function) : builder_(module) {
    auto parameters = component_function ? std::vector<Type>{Type::pointer()}
                                         : std::vector<Type>{};
    auto type = Type::function(Type::void_type(), std::move(parameters));
    function_ = &builder_.core().create_function(std::move(function_name), std::move(type));
    auto& entry = function_->entry_block();
    if (component_function) self_ = &entry.add_argument(Type::pointer(), "self");
    set_insertion_point(entry);
}

BasicBlock& Context::create_block(std::string name) {
    name += "." + std::to_string(next_block_++);
    return builder_.core().create_block(function_->body(), std::move(name));
}

void Context::set_insertion_point(BasicBlock& block) {
    block_ = &block;
    builder_.set_insertion_point(block);
}

bool Context::terminated() const { return block_ && block_->terminator(); }

void Context::branch(BasicBlock& target) {
    if (!terminated()) builder_.core().create_branch(target);
}

void Context::finish() {
    if (!terminated()) builder_.core().create_return();
}
}
