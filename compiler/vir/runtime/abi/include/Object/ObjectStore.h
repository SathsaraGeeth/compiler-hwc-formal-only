#pragma once

#include "../Core/Result.h"
#include "../Core/Value.h"
#include "ClassDescriptor.h"
#include <functional>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vir::runtime {

class ObjectStore {
public:
    using Method = std::function<Result<Value>(ObjectHandle,
                                                std::span<const Value>)>;
    ObjectHandle create(std::string type);
    Result<ObjectHandle> create(std::string_view type,
                                std::span<const Value> arguments);
    bool destroy(ObjectHandle object);
    Result<ObjectHandle> clone(ObjectHandle object);
    Result<bool> compare(ObjectHandle left, ObjectHandle right) const;
    Result<std::string> type(ObjectHandle object) const;
    Result<Value> load(ObjectHandle object, std::string_view field) const;
    Result<void> store(ObjectHandle object, std::string field, Value value);
    Result<Value> call(ObjectHandle object, std::string_view method,
                       std::span<const Value> arguments) const;
    bool add_method(std::string type, std::string name, Method method);
    bool add_class(ClassDescriptor descriptor);

private:
    struct Object {
        std::string type;
        std::unordered_map<std::string, Value> fields;
        ClassDescriptor::NativeObject native;
    };
    mutable std::shared_mutex mutex_;
    uint64_t next_id_ = 1;
    std::unordered_map<uint64_t, Object> objects_;
    std::unordered_map<std::string,
        std::unordered_map<std::string, Method>> methods_;
    std::unordered_map<std::string, ClassDescriptor> classes_;
};

} // namespace vir::runtime
