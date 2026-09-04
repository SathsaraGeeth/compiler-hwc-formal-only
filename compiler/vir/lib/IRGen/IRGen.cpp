#include "IRGen/IRGen.h"
#include <stdexcept>
namespace vir::irgen
{
std::unique_ptr<Module> IRGen::lower(const ModuleSpec &source)const
{
    auto module = std::make_unique<Module>(source.name);
    TypeConverter types;
    for (auto &input:source.functions)
    {
        std::vector<Type> params;
        for (auto &p:input.parameters)
            params.push_back(types.convert(p.second));
        auto &fn = module->add_function(input.name, Type::function(types.convert(input.result_type),
                                                                   params));
        auto &block = fn.entry_block();
        IRBuilder builder(*module);
        builder.set_insertion_point(block);
        SymbolTable symbols;
        for (std::size_t i = 0; i < input.parameters.size(); ++i)
        {
            auto &arg = block.add_argument(params[i], input.parameters[i].first);
            symbols.bind(input.parameters[i].first, arg);
        }
        for (auto &spec:input.operations)
        {
            std::vector<Value *> operands;
            for (auto &name:spec.operands)
            {
                auto *value = symbols.lookup_value(name);
                if (!value)
                    throw std::invalid_argument("unknown VIR value: " + name);
                operands.push_back(value);
            }
            std::vector<Type> results;
            if (!spec.result_type.empty())
                results.push_back(types.convert(spec.result_type));
            auto &op = builder.core().create(spec.name, std::move(operands), std::move(results),
                                             spec.attributes);
            if (!spec.result.empty())
            {
                if (!op.result_count())
                    throw std::invalid_argument("named operation has no result");
                op.result().set_name(spec.result);
                symbols.bind(spec.result, op.result());
            }
        }
        if (!block.terminator())
            builder.core().create_return();
    }
    return module;
}
} // namespace vir::irgen
