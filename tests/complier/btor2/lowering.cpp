#include "HIR/Module.h"
#include "Conversion/SVAToHIR.h"
#include "IR/Verifier.h"
#include "Conversion/EIRToIR.h"
#include "IR/Printer.h"
#include "eir/include/IR/Module.h"
#include <cassert>
#include <memory>
#include <sstream>

int main() {
    emul::frontend::sva::Directive temporal;
    temporal.name = "grant_recurs";
    temporal.property.kind = emul::frontend::sva::PropertyKind::always;
    temporal.property.left = std::make_shared<emul::frontend::sva::Property>();
    temporal.property.left->kind =
        emul::frontend::sva::PropertyKind::eventually;
    auto high_level = emul::btor2::convert_sva_to_hir(
        std::vector<emul::frontend::sva::Directive>{temporal});
    assert(high_level.operations.size() == 1);
    assert(high_level.operations.front().components.size() == 1);
    assert(high_level.operations.front().components.front().opcode ==
           emul::btor2::hir::Opcode::recurrence);

    emul::eir::Program source;
    emul::eir::Module counter;
    counter.name = "counter";
    counter.inputs.push_back({"%enable", "2s<1>"});
    counter.states.push_back({"@count", "2s<8>"});
    counter.operations.push_back({"%old", "2s<8>", "state_read", "@count"});
    counter.operations.push_back({"%next", "2s<8>", "add", "%old, 2s<8>'0x1"});
    counter.operations.push_back({"%selected", "2s<8>", "mux", "%enable, %old, %next"});
    counter.operations.push_back({{}, {}, "state_write", "@count, %selected"});
    source.modules.push_back(std::move(counter));
    auto system = emul::btor2::lower(source);
    emul::btor2::verify(system);
    std::ostringstream output;
    emul::btor2::print(system, output);
    assert(output.str().find(" state ") != std::string::npos);
    assert(output.str().find(" next ") != std::string::npos);

    emul::eir::Program four_state_source;
    emul::eir::Module four_state;
    four_state.name = "four_state";
    four_state.inputs.push_back({"%a", "4s<8>"});
    four_state.inputs.push_back({"%b", "4s<8>"});
    four_state.operations.push_back({"%sum", "4s<8>", "add", "%a, %b"});
    four_state.operations.push_back({"%equal", "4s<1>", "eq", "%sum, 4s<8>'0x3"});
    four_state.operations.push_back({{}, {}, "yield", "%equal"});
    four_state_source.modules.push_back(std::move(four_state));
    auto encoded = emul::btor2::lower(four_state_source);
    emul::btor2::verify(encoded);
    std::ostringstream encoded_output;
    emul::btor2::print(encoded, encoded_output);
    assert(encoded_output.str().find("a.xmask") != std::string::npos);
    assert(encoded_output.str().find("a.zmask") != std::string::npos);
    assert(encoded_output.str().find(" constraint ") != std::string::npos);

    emul::eir::Program hierarchy;
    emul::eir::Module child;
    child.name = "child";
    child.inputs.push_back({"%in", "4s<1>"});
    child.states.push_back({"@saved", "4s<1>"});
    child.operations.push_back({"%old", "4s<1>", "state_read", "@saved"});
    child.operations.push_back({{}, {}, "state_write", "@saved, %in"});
    child.operations.push_back({{}, {}, "yield", "%old"});
    hierarchy.modules.push_back(std::move(child));
    emul::eir::Module parent;
    parent.name = "parent";
    parent.inputs.push_back({"%in", "4s<1>"});
    parent.operations.push_back({"%out", {}, "instance", "@u0, @child(%future)"});
    parent.operations.push_back({"%future", "4s<1>", "not", "%in"});
    parent.operations.push_back({{}, {}, "yield", "%out"});
    hierarchy.modules.push_back(std::move(parent));
    auto flattened = emul::btor2::lower(hierarchy);
    emul::btor2::verify(flattened);
    std::ostringstream hierarchy_output;
    emul::btor2::print(flattened, hierarchy_output);
    assert(hierarchy_output.str().find("u0.saved.data") != std::string::npos);
}
