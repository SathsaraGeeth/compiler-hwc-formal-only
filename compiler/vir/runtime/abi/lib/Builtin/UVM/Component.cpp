#include "Builtin/UVM/Component.h"
#include <stdexcept>

namespace vir::runtime::builtin::uvm {
void Component::register_type(
    std::string type, std::string phase, Callback callback) {
    if (type.empty() || phase.empty() || !callback)
        throw std::invalid_argument("invalid UVM component callback");
    std::lock_guard lock(mutex_);
    callbacks_[std::move(type)].insert_or_assign(
        std::move(phase), std::move(callback));
}

void Component::add(ObjectHandle handle, std::string type, std::string name,
                    ObjectHandle parent) {
    if (!handle || type.empty() || name.empty())
        throw std::invalid_argument("invalid UVM component");
    std::lock_guard lock(mutex_);
    nodes_.push_back({handle, std::move(type), std::move(name), parent});
}

void Component::run(std::string_view phase) {
    // Build may append children. Index traversal deliberately observes the
    // growing component tree, matching UVM's top-down build semantics.
    for (std::size_t index = 0;; ++index) {
        Callback callback;
        ObjectHandle handle;
        {
            std::lock_guard lock(mutex_);
            if (index >= nodes_.size()) break;
            handle = nodes_[index].handle;
            auto type = callbacks_.find(nodes_[index].type);
            if (type != callbacks_.end()) {
                auto found = type->second.find(std::string(phase));
                if (found != type->second.end()) callback = found->second;
            }
        }
        if (callback) callback(handle);
    }
}

std::string Component::path(ObjectHandle handle) const {
    std::lock_guard lock(mutex_);
    std::string result;
    auto current = handle;
    while (current) {
        const Node* found = nullptr;
        for (const auto& node : nodes_)
            if (node.handle == current) { found = &node; break; }
        if (!found) break;
        result = result.empty() ? found->name : found->name + "." + result;
        current = found->parent;
    }
    return result;
}

std::size_t Component::size() const noexcept {
    std::lock_guard lock(mutex_);
    return nodes_.size();
}
void Component::reset() {
    std::lock_guard lock(mutex_);
    nodes_.clear();
    callbacks_.clear();
}
} // namespace vir::runtime::builtin::uvm
