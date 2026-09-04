#pragma once
#include "VIR/Builder.h"
#include "VIR/Intrinsics.h"
namespace vir::irgen
{
class IRBuilder
{
public:
explicit IRBuilder(Module &module) : builder_(module)
{
}
Builder& core()
{
    return builder_;
}
void set_insertion_point(BasicBlock &block)
{
    builder_.set_insertion_point(block);
}
Operation& evaluate_dut(std::string instance);
Operation& export_signal(std::string signal, Value &value);
Operation& import_signal(std::string signal, Value &peek, Type value_type);
Operation& extern_call(std::string name, std::vector<Value *> args, std::vector<Type> results = {});
Operation& object_create(std::string type);
Operation& field_load(Value &object, std::string field, Type result);
Operation& field_store(Value &object, std::string field, Value &value);
Operation& method_call(Value &object, std::string method, std::vector<Value *> args, std::vector<
                           Type> results = {});
Operation& intrinsic(Intrinsic::ID id, std::vector<Value *> arguments);
private: Builder builder_;
};
}
