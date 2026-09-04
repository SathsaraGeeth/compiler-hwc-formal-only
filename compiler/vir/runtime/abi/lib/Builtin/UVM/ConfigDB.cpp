#include "Builtin/UVM/ConfigDB.h"
#include <mutex>

namespace vir::runtime::builtin::uvm {
bool ConfigDB::matches(std::string_view pattern, std::string_view value) {
    const auto wildcard = pattern.find('*');
    if (wildcard == pattern.npos) return pattern == value;
    const auto prefix = pattern.substr(0, wildcard);
    const auto suffix = pattern.substr(wildcard + 1);
    return value.starts_with(prefix) && value.ends_with(suffix) &&
           value.size() >= prefix.size() + suffix.size();
}
void ConfigDB::set(std::string scope, std::string field, Value value) {
    std::unique_lock lock(mutex_);
    entries_.push_back({std::move(scope), std::move(field), std::move(value)});
}
Result<Value> ConfigDB::get(std::string_view instance, std::string_view field) const {
    std::shared_lock lock(mutex_);
    for (auto item = entries_.rbegin(); item != entries_.rend(); ++item)
        if (item->field == field && matches(item->scope, instance))
            return Result<Value>::completed(item->value);
    return Result<Value>::failed("UVM config field not found: " + std::string(field));
}
void ConfigDB::reset() { std::unique_lock lock(mutex_); entries_.clear(); }
} // namespace vir::runtime::builtin::uvm
