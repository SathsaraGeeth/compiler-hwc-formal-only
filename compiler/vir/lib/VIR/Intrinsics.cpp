#include "VIR/Intrinsics.h"
#include "VIR/Module.h"
#include <stdexcept>
namespace vir::Intrinsic {
std::string_view get_name(ID id) {
    switch (id) {
    case ID::time_delay: return "vir.runtime.time.delay";
    case ID::assert_check: return "vir.runtime.assert.check";
    case ID::uvm_objection_raise: return "vir.runtime.uvm.objection.raise";
    case ID::uvm_objection_drop: return "vir.runtime.uvm.objection.drop";
    case ID::not_intrinsic: return {};
    }
    return {};
}
ID lookup_id(std::string_view name) {
    if (name == get_name(ID::time_delay)) return ID::time_delay;
    if (name == get_name(ID::assert_check)) return ID::assert_check;
    if (name == get_name(ID::uvm_objection_raise)) return ID::uvm_objection_raise;
    if (name == get_name(ID::uvm_objection_drop)) return ID::uvm_objection_drop;
    return ID::not_intrinsic;
}
Function& get_declaration(Module& module, ID id, std::vector<Type> argument_types) {
    auto name = std::string(get_name(id));
    if (name.empty()) throw std::invalid_argument("not a VIR intrinsic");
    if (auto* function = module.find_function(name)) return *function;
    return module.add_function(
        std::move(name), Type::function(Type::void_type(), std::move(argument_types)));
}
}
