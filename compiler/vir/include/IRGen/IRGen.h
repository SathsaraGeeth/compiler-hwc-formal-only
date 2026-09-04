#pragma once
#include "IRGen/ControlFlowBuilder.h"
#include "IRGen/SymbolTable.h"
#include "IRGen/TypeConverter.h"
#include <memory>
#include <string>
#include <vector>
namespace vir::irgen
{
struct OperationSpec {
    std::string name;
    std::vector<std::string> operands;
    std::string result;
    std::string result_type;
    std::map<std::string, Attribute> attributes;
};
struct FunctionSpec {
    std::string name;
    std::string result_type = "void";
    std::vector<std::pair<std::string, std::string> > parameters;
    std::vector<OperationSpec> operations;
};
struct ModuleSpec {
    std::string name;
    std::vector<FunctionSpec> functions;
};
class IRGen
{
public:
std::unique_ptr<Module> lower(const ModuleSpec &source) const;
};
}
