/* Builds a real BTOR2 model from formal EIR and adds the adder obligation. */

#include "frontend/frontend.h"
#include "eir/include/Lowering/Frontend.h"
#include "eir/include/IR/Verifier.h"
#include "Lowering/TwoStateEIRLowering.h"
#include "Lowering/Formal/lower_sva.h"
#include "frontend/SVA/Lowering/directive.h"
#include <cassert>
#include <fstream>

int main(int argc, char** argv) {
    assert(argc == 4);
    emul::frontend::FrontendInput input;
    input.top = "formal_adder_properties";
    input.sources.emplace_back(argv[1]);
    input.sources.emplace_back(argv[2]);
    auto design = emul::frontend::Frontend().elaborate(input);
    auto hardware = emul::lowering::to_eir(design);
    emul::eir::verify(hardware);
    auto system = emul::btor2::lower_two_state(hardware);

    auto directive = emul::frontend::sva::lower_named_directive(
        design.root(), input.top, "adder_correct");
    emul::formal::lower_sva_directive(directive, system);

    std::ofstream output(argv[3]);
    assert(output);
    system.builder.write(output);
}
