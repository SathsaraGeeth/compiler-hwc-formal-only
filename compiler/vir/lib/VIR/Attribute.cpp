#include "VIR/Attribute.h"
#include <iomanip>
#include <sstream>
namespace vir
{
std::string Attribute::str() const
{
    return std::visit([](const auto &value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>)
                return "unit";
            else if constexpr (std::is_same_v<T, bool>)
                return value ? "true" : "false";
            else if constexpr (std::is_same_v<T, std::int64_t>)
                return std::to_string(value);
            else if constexpr (std::is_same_v<T, double>)
            {
                std::ostringstream os;
                os << std::setprecision(17) << value;
                auto result = os.str();
                if (result.find_first_of(".eE") == result.npos) result += ".0";
                return result;
            }
            else if constexpr (std::is_same_v<T, std::string>)
                return '"' + value + '"';
            else if constexpr (std::is_same_v<T, Type>)
                return value.str();
            else
            {
                std::string out = "[";
                for (std::size_t i = 0; i < value.size(); ++i)
                {
                    if (i)
                        out += ", ";
                    out += value[i].str();
                }
                return out + ']';
            }
        }, value_);
}
} // namespace vir
