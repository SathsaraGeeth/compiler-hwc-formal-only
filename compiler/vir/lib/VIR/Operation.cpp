#include "VIR/Operation.h"
#include "VIR/Region.h"
#include <stdexcept>
namespace vir
{
Operation::Operation(std::string name, std::vector<Value *> operands, std::vector<Type> result_types
                     , std::map<std::string, Attribute> attributes)
    : name_(std::move(name)), operands_(std::move(operands)), attributes_(std::move(attributes))
{
    for (std::size_t i = 0; i < operands_.size(); ++i)
    {
        if (!operands_[i])
            throw std::invalid_argument("null operand");
        operands_[i]->add_use(this, i);
    }
    for (std::size_t i = 0; i < result_types.size(); ++i)
        results_.push_back(std::unique_ptr<Value>(new Value(std::move(result_types[i]), this, i)));
}
Operation::~Operation()
{
    for (std::size_t i = 0; i < operands_.size(); ++i)
        operands_[i]->remove_use(this, i);
}
const Attribute * Operation::attribute(const std::string &name) const
{
    auto it = attributes_.find(name);
    return it == attributes_.end()?nullptr:&it->second;
}
void Operation::set_attribute(std::string name, Attribute value)
{
    attributes_.insert_or_assign(std::move(name), std::move(value));
}
void Operation::set_operand(std::size_t index, Value &value)
{
    operands_.at(index)->remove_use(this, index);
    operands_[index] = &value;
    value.add_use(this, index);
}
Region& Operation::add_region()
{
    regions_.push_back(std::make_unique<Region>(this));
    return *regions_.back();
}
bool Operation::is_terminator() const
{
    return name_ == "ret" || name_ == "br" || name_ == "cond_br" || name_ == "switch" || name_ ==
           "unreachable";
}
} // namespace vir
