#pragma once
#include "../../Core/Value.h"
#include <optional>
#include <string>
#include <string_view>

namespace vir::runtime::builtin {
class Type {
public:
    static std::string name(const Value& value);
    static bool is_instance(const Value& value, std::string_view type);
    static std::optional<Value> cast(const Value& value, std::string_view type);
};
}
