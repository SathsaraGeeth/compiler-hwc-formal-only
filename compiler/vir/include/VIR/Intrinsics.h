#pragma once
#include "VIR/Type.h"
#include <string_view>
#include <vector>
namespace vir { class Function; class Module; namespace Intrinsic {
enum class ID : unsigned {
    not_intrinsic = 0,
    time_delay,
    assert_check,
    uvm_objection_raise,
    uvm_objection_drop
};
std::string_view get_name(ID id);
ID lookup_id(std::string_view name);
Function& get_declaration(Module& module, ID id, std::vector<Type> argument_types);
} }
