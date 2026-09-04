#pragma once

#include "VIR/Module.h"

namespace emul::frontend { class ElaboratedDesign; class SemanticNode; }
namespace vir::irgen::frontend {
void lower_transport(Module& module,
                     emul::frontend::SemanticNode host,
                     const emul::frontend::ElaboratedDesign& design);
}
