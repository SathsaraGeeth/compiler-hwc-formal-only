#pragma once

#include "Object/ObjectStore.h"
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vir::runtime::builtin::uvm {

// Queue storage lives in ObjectStore. The map only resolves the stable
// (component owner, member name) identity to its serializable object handle.
class Collection {
public:
    explicit Collection(ObjectStore& objects) : objects_(objects) {}
    void push(ObjectHandle owner, std::string_view name, Value value);
    Value get(ObjectHandle owner, std::string_view name, std::uint64_t index);
    void erase(ObjectHandle owner, std::string_view name, std::uint64_t index);
    std::uint64_t size(ObjectHandle owner, std::string_view name);
    void reset();

private:
    ObjectHandle resolve(ObjectHandle owner, std::string_view name);
    static std::string element(std::uint64_t index);
    ObjectStore& objects_;
    std::mutex mutex_;
    std::unordered_map<std::string, ObjectHandle> collections_;
};

} // namespace vir::runtime::builtin::uvm
