#include "External/Registry.h"
#include <mutex>

namespace vir::runtime {

bool ExternalRegistry::add(std::string name, Function function) {
    if (name.empty() || !function) return false;
    std::unique_lock lock(mutex_);
    return functions_.emplace(std::move(name), std::move(function)).second;
}

bool ExternalRegistry::remove(std::string_view name) {
    std::unique_lock lock(mutex_);
    return functions_.erase(std::string(name)) != 0;
}

Result<Value> ExternalRegistry::call(std::string_view name,
                                     std::span<const Value> arguments) const {
    Function function;
    {
        std::shared_lock lock(mutex_);
        auto found = functions_.find(std::string(name));
        if (found == functions_.end())
            return Result<Value>::failed("unknown external function " + std::string(name));
        function = found->second;
    }
    return function(arguments);
}

} // namespace vir::runtime
