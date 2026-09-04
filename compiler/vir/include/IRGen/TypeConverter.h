#pragma once
#include "VIR/Type.h"
#include <string_view>
namespace vir::irgen
{
class TypeConverter
{
public: Type convert(std::string_view spelling) const;
};
}
