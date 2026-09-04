#include "IRGen/IRBuilder.h"
namespace vir::irgen
{
Operation& IRBuilder::evaluate_dut(std::string instance)
{
    return builder_.create("evaluate_dut", {}, {Type::integer(32), Type::object("job")}, {{
                               "instance", std::move(instance)}});
}
Operation& IRBuilder::export_signal(std::string signal, Value &value)
{
    return builder_.create("export", {&value}, {Type::integer(32)}, {{"signal", std::move(signal)}})
    ;
}
Operation& IRBuilder::import_signal(std::string signal, Value &peek, Type value_type)
{
    return builder_.create("import", {&peek},
        {Type::integer(32), value_type, value_type, std::move(value_type)}, {{
                               "signal", std::move(signal)}});
}
Operation& IRBuilder::extern_call(std::string name, std::vector<Value *> args, std::vector<Type>
                                  results)
{
    return builder_.create("extern_call", std::move(args), std::move(results), {{"callee", std::move
                                   (name)}});
}
Operation& IRBuilder::object_create(std::string type)
{
    return builder_.create("object_create", {}, {Type::object(type)}, {{"type", std::move(type)}});
}
Operation& IRBuilder::field_load(Value &object, std::string field, Type result)
{
    return builder_.create("field_load", {&object}, {std::move(result)}, {{"field", std::move(field)
                           }});
}
Operation& IRBuilder::field_store(Value &object, std::string field, Value &value)
{
    return builder_.create("field_store", {&object, &value}, {}, {{"field", std::move(field)}});
}
Operation& IRBuilder::method_call(Value &object, std::string method, std::vector<Value *> args,
                                  std::vector<Type> results)
{
    args.insert(args.begin(), &object);
    return builder_.create("method_call", std::move(args), std::move(results), {{"method", std::move
                                   (method)}});
}
Operation& IRBuilder::intrinsic(Intrinsic::ID id, std::vector<Value *> arguments)
{
    std::vector<Type> types;
    for (auto *argument : arguments)
        types.push_back(argument->type());
    Intrinsic::get_declaration(builder_.module(), id, std::move(types));
    return builder_.create("call", std::move(arguments), {},
                           {{"callee", std::string(Intrinsic::get_name(id))}});
}
} // namespace vir::irgen
