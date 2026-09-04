#pragma once

#include "Core/Result.h"
#include "Core/Value.h"
#include <shared_mutex>
#include <string>
#include <vector>

namespace vir::runtime::builtin::uvm {

class ConfigDB {
public:
    void set(std::string scope, std::string field, Value value);
    Result<Value> get(std::string_view instance, std::string_view field) const;
    void reset();

private:
    struct Entry { std::string scope, field; Value value; };
    static bool matches(std::string_view pattern, std::string_view value);
    mutable std::shared_mutex mutex_;
    std::vector<Entry> entries_;
};

} // namespace vir::runtime::builtin::uvm
