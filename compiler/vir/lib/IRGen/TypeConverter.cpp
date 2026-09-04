#include "IRGen/TypeConverter.h"
#include <charconv>
#include <stdexcept>
namespace vir::irgen
{
Type TypeConverter::convert(std::string_view text) const
{
    if (text == "void")
        return Type::void_type();
    if (text == "float" || text == "double" || text == "half" || text == "bfloat" || text ==
        "fp128")
        return Type::floating(std::string(text));
    if (text == "ptr")
        return Type::pointer();
    if (text == "label")
        return Type::label();
    if (text.size() > 1 && text[0] == 'i')
    {
        unsigned width = 0;
        auto [p, e] = std::from_chars(text.data() + 1, text.data() + text.size(), width);
        if (e == std::errc{} && p == text.data() + text.size())
            return Type::integer(width);
    }
    if (text.starts_with('!'))
        return Type::object(std::string(text.substr(1)));
    return Type::opaque(std::string(text));
}
} // namespace vir::irgen
