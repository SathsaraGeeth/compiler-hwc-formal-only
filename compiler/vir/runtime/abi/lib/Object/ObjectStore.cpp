#include "Object/ObjectStore.h"
#include <mutex>

namespace vir::runtime {

ObjectHandle ObjectStore::create(std::string type) {
    if (type.empty()) return {};
    std::unique_lock lock(mutex_);
    const auto id = next_id_++;
    objects_.emplace(id, Object{std::move(type), {}, {}});
    return {id};
}

Result<ObjectHandle> ObjectStore::create(
    std::string_view type, std::span<const Value> arguments) {
    ClassDescriptor descriptor;
    {
        std::shared_lock lock(mutex_);
        auto found = classes_.find(std::string(type));
        if (found == classes_.end())
            return Result<ObjectHandle>::failed("unknown object type " + std::string(type));
        descriptor = found->second;
    }
    auto native = descriptor.construct(arguments);
    if (!native.ready()) return native.state() == State::retry
        ? Result<ObjectHandle>::retry()
        : Result<ObjectHandle>::failed(native.error());
    std::unique_lock lock(mutex_);
    const auto id = next_id_++;
    objects_.emplace(id, Object{descriptor.name, {}, native.value()});
    return Result<ObjectHandle>::completed(ObjectHandle{id});
}

bool ObjectStore::destroy(ObjectHandle object) {
    std::unique_lock lock(mutex_);
    return objects_.erase(object.value) != 0;
}

Result<ObjectHandle> ObjectStore::clone(ObjectHandle object) {
    std::unique_lock lock(mutex_);
    auto found = objects_.find(object.value);
    if (found == objects_.end()) return Result<ObjectHandle>::failed("invalid object handle");
    const auto id = next_id_++;
    objects_.emplace(id, found->second);
    return Result<ObjectHandle>::completed(ObjectHandle{id});
}

Result<bool> ObjectStore::compare(ObjectHandle left, ObjectHandle right) const {
    std::shared_lock lock(mutex_);
    auto lhs = objects_.find(left.value), rhs = objects_.find(right.value);
    if (lhs == objects_.end() || rhs == objects_.end())
        return Result<bool>::failed("invalid object handle");
    return Result<bool>::completed(lhs->second.type == rhs->second.type &&
                                   lhs->second.fields == rhs->second.fields);
}

Result<std::string> ObjectStore::type(ObjectHandle object) const {
    std::shared_lock lock(mutex_);
    auto found = objects_.find(object.value);
    if (found == objects_.end()) return Result<std::string>::failed("invalid object handle");
    return Result<std::string>::completed(found->second.type);
}

Result<Value> ObjectStore::load(ObjectHandle object, std::string_view field) const {
    std::shared_lock lock(mutex_);
    auto found = objects_.find(object.value);
    if (found == objects_.end()) return Result<Value>::failed("invalid object handle");
    auto value = found->second.fields.find(std::string(field));
    if (value == found->second.fields.end())
        return Result<Value>::failed("unknown object field " + std::string(field));
    return Result<Value>::completed(value->second);
}

Result<void> ObjectStore::store(ObjectHandle object, std::string field, Value value) {
    if (field.empty()) return Result<void>::failed("empty object field name");
    std::unique_lock lock(mutex_);
    auto found = objects_.find(object.value);
    if (found == objects_.end()) return Result<void>::failed("invalid object handle");
    found->second.fields.insert_or_assign(std::move(field), std::move(value));
    return Result<void>::completed();
}

Result<Value> ObjectStore::call(ObjectHandle object, std::string_view method,
                                std::span<const Value> arguments) const {
    Method function;
    ClassDescriptor::Method native_function;
    ClassDescriptor::NativeObject native;
    {
        std::shared_lock lock(mutex_);
        auto found = objects_.find(object.value);
        if (found == objects_.end()) return Result<Value>::failed("invalid object handle");
        auto methods = methods_.find(found->second.type);
        if (methods != methods_.end()) {
            auto selected = methods->second.find(std::string(method));
            if (selected != methods->second.end()) function = selected->second;
        }
        auto descriptor = classes_.find(found->second.type);
        if (descriptor != classes_.end()) {
            auto selected = descriptor->second.methods.find(std::string(method));
            if (selected != descriptor->second.methods.end()) {
                native_function = selected->second;
                native = found->second.native;
            }
        }
    }
    if (native_function) return native_function(native, arguments);
    if (function) return function(object, arguments);
    return Result<Value>::failed("unknown object method " + std::string(method));
}

bool ObjectStore::add_class(ClassDescriptor descriptor) {
    if (descriptor.name.empty() || !descriptor.construct) return false;
    std::unique_lock lock(mutex_);
    return classes_.emplace(descriptor.name, std::move(descriptor)).second;
}

bool ObjectStore::add_method(std::string type, std::string name, Method method) {
    if (type.empty() || name.empty() || !method) return false;
    std::unique_lock lock(mutex_);
    return methods_[std::move(type)].emplace(std::move(name), std::move(method)).second;
}

} // namespace vir::runtime
