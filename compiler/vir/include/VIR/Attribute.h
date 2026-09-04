#pragma once

#include "VIR/Type.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace vir
{

class Attribute
{
public:
using Array = std::vector<Attribute>;
using Storage = std::variant<std::monostate, bool, std::int64_t, double, std::string, Type, Array>;
Attribute() = default;
Attribute(bool value) : value_(value)
{
}
Attribute(std::int64_t value) : value_(value)
{
}
Attribute(double value) : value_(value)
{
}
Attribute(std::string value) : value_(std::move(value))
{
}
Attribute(const char *value) : value_(std::string(value))
{
}
Attribute(Type value) : value_(std::move(value))
{
}
Attribute(Array value) : value_(std::move(value))
{
}
const Storage& value() const
{
    return value_;
}
std::string str() const;
friend bool operator==(const Attribute &, const Attribute &) = default;
private:
Storage value_;
};

} // namespace vir
