#pragma once
#include "VIR/Module.h"
#include <memory>
#include <string_view>
namespace emul::frontend
{
class ElaboratedDesign;
}
namespace vir::irgen
{
std::unique_ptr<Module> lower_frontend(
    const emul::frontend::ElaboratedDesign& design,
    std::string_view configured_uvm_test = {});
}
