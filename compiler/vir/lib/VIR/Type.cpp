#include "VIR/Type.h"
#include <sstream>
#include <stdexcept>

namespace vir
{
struct Type::Impl {
    Kind kind;
    std::uint32_t width = 0;
    std::uint32_t address_space = 0;
    std::uint64_t size = 0;
    std::string name;
    std::vector<Type> elements;
    Type result;
    bool packed = false;
    bool variadic = false;
};

Type Type::make(Kind kind, std::uint32_t width, std::uint32_t address_space,
                std::uint64_t size, std::string name, std::vector<Type> elements,
                Type result, bool packed, bool variadic)
{
    auto impl = std::make_shared<Impl>();
    impl->kind = kind;
    impl->width = width;
    impl->address_space = address_space;
    impl->size = size;
    impl->name = std::move(name);
    impl->elements = std::move(elements);
    impl->result = std::move(result);
    impl->packed = packed;
    impl->variadic = variadic;
    return Type(std::move(impl));
}
Type Type::void_type()
{
    return make(Kind::Void, 0, 0, 0, {}, {}, {}, false, false);
}
Type Type::integer(std::uint32_t width)
{
    if (!width)
        throw std::invalid_argument("integer width is zero");
    return make(Kind::Integer, width, 0, 0, {}, {}, {}, false, false);
}
Type Type::floating(std::string name)
{
    return make(Kind::Float, 0, 0, 0, std::move(name), {}, {}, false, false);
}
Type Type::pointer(std::uint32_t space)
{
    return make(Kind::Pointer, 0, space, 0, {}, {}, {}, false, false);
}
Type Type::array(std::uint64_t size, Type element)
{
    return make(Kind::Array, 0, 0, size, {}, {std::move(element)}, {}, false, false);
}
Type Type::structure(std::vector<Type> fields, bool packed)
{
    return make(Kind::Struct, 0, 0, 0, {}, std::move(fields), {}, packed, false);
}
Type Type::function(Type result, std::vector<Type> params, bool variadic)
{
    return make(Kind::Function, 0, 0, 0, {}, std::move(params), std::move(result), false, variadic);
}
Type Type::label()
{
    return make(Kind::Label, 0, 0, 0, {}, {}, {}, false, false);
}
Type Type::object(std::string name)
{
    return make(Kind::Object, 0, 0, 0, std::move(name), {}, {}, false, false);
}
Type Type::opaque(std::string spelling)
{
    return make(Kind::Opaque, 0, 0, 0, std::move(spelling), {}, {}, false, false);
}
Type::Kind Type::kind() const
{
    return impl_->kind;
}
std::uint32_t Type::width() const
{
    return impl_->width;
}
std::uint32_t Type::address_space() const
{
    return impl_->address_space;
}
std::uint64_t Type::size() const
{
    return impl_->size;
}
const std::string& Type::name() const
{
    return impl_->name;
}
const std::vector<Type>& Type::elements() const
{
    return impl_->elements;
}
const Type& Type::result() const
{
    return impl_->result;
}
bool Type::packed() const
{
    return impl_->packed;
}
bool Type::variadic() const
{
    return impl_->variadic;
}
std::string Type::str() const
{
    if (!impl_)
        return "<invalid>";
    std::ostringstream os;
    switch (kind()) {
    case Kind::Void: return "void";
    case Kind::Integer: return "i" + std::to_string(width());
    case Kind::Float: return name();
    case Kind::Pointer: os << "ptr";
        if (address_space())
            os << " addrspace(" << address_space() << ')';
        break;
    case Kind::Array: os << '[' << size() << " x " << elements()[0].str() << ']';
        break;
    case Kind::Struct: os << (packed() ? "<{" : "{");
        for (std::size_t i = 0; i < elements().size(); ++i)
        {
            if (i)
                os << ", ";
            os << elements()[i].str();
        }
        os << (packed() ? "}>" : "}");
        break;
    case Kind::Function: os << result().str() << " (";
        for (std::size_t i = 0; i < elements().size(); ++i)
        {
            if (i)
                os << ", ";
            os << elements()[i].str();
        }
        if (variadic())
            os << (elements().empty()?"...":", ...");
        os << ')';
        break;
    case Kind::Label: return "label";
    case Kind::Object: return '!' + name();
    case Kind::Opaque: return name();
    } return os.str();
}
bool operator==(const Type &a, const Type &b)
{
    return a.str() == b.str();
}
} // namespace vir
