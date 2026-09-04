#pragma once

#include "Core/Value.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace vir::runtime::builtin::uvm {

class Component {
public:
    using Callback = std::function<void(ObjectHandle)>;

    void register_type(std::string type, std::string phase, Callback callback);
    void add(ObjectHandle handle, std::string type, std::string name,
             ObjectHandle parent = {});
    void run(std::string_view phase);
    std::string path(ObjectHandle handle) const;
    std::size_t size() const noexcept;
    void reset();

private:
    struct Node {
        ObjectHandle handle;
        std::string type;
        std::string name;
        ObjectHandle parent;
    };
    mutable std::mutex mutex_;
    std::vector<Node> nodes_;
    std::unordered_map<std::string,
        std::unordered_map<std::string, Callback>> callbacks_;
};

} // namespace vir::runtime::builtin::uvm
