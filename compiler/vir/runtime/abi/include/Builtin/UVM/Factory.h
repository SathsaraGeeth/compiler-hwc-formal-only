#pragma once

#include "Object/ObjectStore.h"
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace vir::runtime::builtin::uvm {

class Factory {
public:
    explicit Factory(ObjectStore& objects) : objects_(objects) {}
    void set_override(std::string requested, std::string replacement);
    Result<ObjectHandle> create(std::string_view type, std::string name,
                                ObjectHandle parent = {});
    void reset();

private:
    ObjectStore& objects_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> overrides_;
};

} // namespace vir::runtime::builtin::uvm
