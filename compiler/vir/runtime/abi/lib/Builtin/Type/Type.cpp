#include "Builtin/Type/Type.h"

namespace vir::runtime::builtin {
std::string Type::name(const Value& value) {
    static constexpr const char* names[]{"void", "bool", "i64", "u64", "string", "signal_value", "object"};
    return names[value.index()];
}
bool Type::is_instance(const Value& value, std::string_view type) { return name(value) == type; }
std::optional<Value> Type::cast(const Value& value, std::string_view type) {
    if (is_instance(value, type)) return value;
    if (type == "u64" && std::holds_alternative<int64_t>(value)) return Value{uint64_t(std::get<int64_t>(value))};
    if (type == "i64" && std::holds_alternative<uint64_t>(value)) return Value{int64_t(std::get<uint64_t>(value))};
    return {};
}
}
