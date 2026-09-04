#pragma once

#include "VIR/Function.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace vir
{
class Module
{
public:
struct SignalBinding {
    std::string machine_signal;
    std::string host_signal;
    bool clock = false;
};
explicit Module(std::string name = {}) : name_(std::move(name))
{
}
Function& add_function(std::string name, Type type);
Function * find_function(const std::string &name) const;
const std::string& name() const
{
    return name_;
}
const std::vector<std::unique_ptr<Function> >& functions() const
{
    return functions_;
}
void add_signal_binding(SignalBinding binding) { signal_bindings_.push_back(std::move(binding)); }
const std::vector<SignalBinding>& signal_bindings() const { return signal_bindings_; }
void set_attribute(std::string name, Attribute value);
const std::map<std::string, Attribute>& attributes() const
{
    return attributes_;
}
private:
std::string name_;
std::vector<std::unique_ptr<Function> > functions_;
std::map<std::string, Function *> symbols_;
std::map<std::string, Attribute> attributes_;
std::vector<SignalBinding> signal_bindings_;
};
} // namespace vir
