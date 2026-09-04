/*
 * Verifies the EIR to MIR boundary independently of target encoding.
 *
 * The test checks structural lowering and operand classification: EIR values
 * become mutable MIR registers, state names remain state operands, constants
 * remain literals, and plain integer parameters become immediates.
 */

#include "eir/include/Lowering/LowerToMIR.h"

#include <cassert>
#include <utility>

int main() {
    emul::eir::Module source;
    source.name = "mir_lowering";
    source.inputs.push_back({"%input", "4s<8>"});
    source.results.push_back({"output", "4s<8>"});
    source.states.push_back({"@counter", "4s<8>"});
    source.operations.push_back({"%sum", "4s<8>", "add", "%input, 4s<8>'01"});
    source.operations.push_back({"%next", "4s<8>", "slice", "%sum, 7, 0"});
    source.operations.push_back({"", "", "state_write", "@counter, %next"});
    source.operations.push_back({"", "", "yield", "%next"});

    emul::eir::Program program;
    program.modules.push_back(std::move(source));

    const auto machine = emul::eir::lowering::lower_to_mir(program);
    assert(machine.functions.size() == 1);
    const auto& function = machine.root();
    assert(function.blocks.size() == 1);
    const auto& instructions = function.blocks.front().instructions;
    assert(instructions.size() == 4);

    using Kind = emul::mir::MachineOperandKind;
    assert(instructions[0].operands[0].kind == Kind::virtual_register);
    assert(instructions[0].operands[1].kind == Kind::literal);
    assert(instructions[1].operands[1].kind == Kind::immediate);
    assert(instructions[1].operands[2].kind == Kind::immediate);
    assert(instructions[2].operands[0].kind == Kind::state);
    assert(instructions[2].operands[1].kind == Kind::virtual_register);
}
