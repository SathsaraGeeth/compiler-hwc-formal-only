#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vir
{

class Type
{
public:
enum class Kind {
    Void, Integer, Float, Pointer, Array, Struct, Function, Label, Object, Opaque
};

Type() = default;

static Type void_type();
static Type integer(std::uint32_t width);
static Type floating(std::string name);
static Type pointer(std::uint32_t address_space = 0);
static Type array(std::uint64_t size, Type element);
static Type structure(std::vector<Type> fields, bool packed = false);
static Type function(Type result, std::vector<Type> parameters, bool variadic = false);
static Type label();
static Type object(std::string name);
static Type opaque(std::string spelling);

Kind kind() const;
std::uint32_t width() const;
std::uint32_t address_space() const;
std::uint64_t size() const;
const std::string& name() const;
const std::vector<Type>& elements() const;
const Type& result() const;
bool packed() const;
bool variadic() const;
std::string str() const;
explicit operator bool() const
{
    return static_cast<bool>(impl_);
}
friend bool operator==(const Type &, const Type &);

private:
struct Impl;
explicit Type(std::shared_ptr<const Impl> impl) : impl_(std::move(impl))
{
}
static Type make(Kind, std::uint32_t, std::uint32_t, std::uint64_t,
                 std::string, std::vector<Type>, Type, bool, bool);
std::shared_ptr<const Impl> impl_;
};

} // namespace vir
