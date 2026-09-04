/*
 * compiler/eir/lib/Lowering/LowerToMIR.cpp
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 *
 */

#include "../../include/Lowering/LowerToMIR.h"
#include "../../include/Lowering/Flatten.h"
#include "../../include/MIR/Verifier.h"
#include <stdexcept>

namespace emul::eir::lowering {
namespace {
std::vector<std::string> split(std::string_view text) {
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t index = 0; index <= text.size(); ++index) {
        if (index < text.size() && text[index] != ',') continue;
        auto item = std::string(text.substr(start, index - start));
        auto first = item.find_first_not_of(' '), last = item.find_last_not_of(' ');
        result.push_back(first == item.npos ? "" : item.substr(first, last - first + 1));
        start = index + 1;
    }
    if (result.size() == 1 && result.front().empty()) result.clear();
    return result;
}

}

mir::MachineModule lower_to_mir(const Program& program) {
    auto source = flatten(program);
    mir::MachineFunction function;
    function.name = source.name;
    function.inputs = std::move(source.inputs);
    function.results = std::move(source.results);
    function.states = std::move(source.states);
    mir::MachineBlock block{.name = "entry"};
    for (auto& operation : source.operations) {
        auto opcode = mir::parse_opcode(operation.opcode);
        if (!opcode)
            throw std::runtime_error(
                "MIR does not define EIR operation " + operation.opcode);
        mir::MachineInstr instruction{operation.result, operation.result_type,
                                      *opcode, {}};
        for (auto& text : split(operation.operands))
            instruction.operands.push_back(mir::MachineOperand::classify(std::move(text)));
        block.instructions.push_back(std::move(instruction));
    }
    function.blocks.push_back(std::move(block));
    mir::MachineModule result;
    result.functions.push_back(std::move(function));
    mir::verify(result);
    return result;
}
}
