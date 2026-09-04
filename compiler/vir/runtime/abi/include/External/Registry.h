#pragma once

#include "../Core/Result.h"
#include "../Core/Value.h"
#include <functional>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vir::runtime {

class ExternalRegistry {
public:
    using Function = std::function<Result<Value>(std::span<const Value>)>;

    bool add(std::string name, Function function);
    bool remove(std::string_view name);
    Result<Value> call(std::string_view name,
                       std::span<const Value> arguments) const;
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Function> functions_;
};

} // namespace vir::runtime
