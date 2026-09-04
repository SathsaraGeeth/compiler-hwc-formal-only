#include "VIR/IntrinsicInst.h"
namespace vir {
namespace {
std::string callee(const Operation& operation) {
    auto* attribute = operation.attribute("callee");
    if (!attribute) return {};
    auto* name = std::get_if<std::string>(&attribute->value());
    return name ? *name : std::string{};
}
}
bool IntrinsicInst::classof(const Operation& operation) {
    return operation.name() == "call" &&
           Intrinsic::lookup_id(callee(operation)) != Intrinsic::ID::not_intrinsic;
}
Intrinsic::ID IntrinsicInst::intrinsic_id() const {
    return Intrinsic::lookup_id(callee(operation_));
}
}
