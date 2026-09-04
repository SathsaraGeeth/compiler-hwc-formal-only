#include "Builtin/UVM/Collection.h"
#include <stdexcept>

namespace vir::runtime::builtin::uvm {
namespace {
std::uint64_t unsigned_value(const Result<Value>& result) {
    if (!result.ready()) throw std::runtime_error("invalid serialized UVM collection");
    if (auto value = std::get_if<std::uint64_t>(&result.value())) return *value;
    if (auto value = std::get_if<std::int64_t>(&result.value())) return *value;
    throw std::runtime_error("invalid serialized UVM collection size");
}
}

std::string Collection::element(std::uint64_t index) {
    return "element." + std::to_string(index);
}
ObjectHandle Collection::resolve(ObjectHandle owner, std::string_view name) {
    if (!owner || name.empty()) throw std::invalid_argument("invalid UVM collection identity");
    auto key = std::to_string(owner.value) + ":" + std::string(name);
    std::lock_guard lock(mutex_);
    auto found = collections_.find(key);
    if (found != collections_.end()) return found->second;
    auto object = objects_.create("uvm_collection");
    objects_.store(object, "owner", owner);
    objects_.store(object, "name", std::string(name));
    objects_.store(object, "size", std::uint64_t{0});
    collections_.emplace(std::move(key), object);
    return object;
}
void Collection::push(ObjectHandle owner, std::string_view name, Value value) {
    auto object = resolve(owner, name);
    auto count = unsigned_value(objects_.load(object, "size"));
    objects_.store(object, element(count), std::move(value));
    objects_.store(object, "size", count + 1);
}
Value Collection::get(ObjectHandle owner, std::string_view name, std::uint64_t index) {
    auto object = resolve(owner, name);
    auto count = unsigned_value(objects_.load(object, "size"));
    if (index >= count) throw std::out_of_range("UVM collection index");
    auto result = objects_.load(object, element(index));
    if (!result.ready()) throw std::runtime_error("missing serialized UVM collection element");
    return result.value();
}
void Collection::erase(ObjectHandle owner, std::string_view name, std::uint64_t index) {
    auto object = resolve(owner, name);
    auto count = unsigned_value(objects_.load(object, "size"));
    if (index >= count) throw std::out_of_range("UVM collection index");
    for (auto current = index; current + 1 < count; ++current) {
        auto next = objects_.load(object, element(current + 1));
        if (!next.ready()) throw std::runtime_error("missing serialized UVM collection element");
        objects_.store(object, element(current), next.value());
    }
    objects_.store(object, "size", count - 1);
}
std::uint64_t Collection::size(ObjectHandle owner, std::string_view name) {
    return unsigned_value(objects_.load(resolve(owner, name), "size"));
}
void Collection::reset() {
    std::lock_guard lock(mutex_);
    for (const auto& [_, object] : collections_) objects_.destroy(object);
    collections_.clear();
}
} // namespace vir::runtime::builtin::uvm
