/*
 * compiler/btor2/lib/IR/Printer.cpp
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

#include "../../include/IR/Printer.h"
#include <map>
#include <ostream>
#include <vector>
namespace emul::btor2 {
void print(const Module& module, std::ostream& output) {
    std::map<std::uint32_t, std::uint32_t> sorts;
    std::vector<std::uint32_t> sort_widths;
    std::uint32_t next = 1;
    for (const auto& operation : module.operations)
        if (!sorts.contains(operation.width)) {
            sorts[operation.width] = next++;
            sort_widths.push_back(operation.width);
        }
    for (auto width : sort_widths)
        output << sorts.at(width) << " sort bitvec " << width << '\n';
    std::vector<std::uint32_t> ids(module.operations.size() + 1);
    for (NodeId node_id = 1; node_id <= module.operations.size(); ++node_id) {
        ids[node_id] = next++;
        const auto& node = module.get(node_id);
        output << ids[node_id] << ' ' << node.opcode << ' ' << sorts.at(node.width);
        if (node.opcode == "const") output << ' ' << node.bits;
        for (auto operand : node.operands) output << ' ' << ids.at(operand);
        for (auto immediate : node.immediates) output << ' ' << immediate;
        if (!node.symbol.empty()) output << ' ' << node.symbol;
        output << '\n';
    }
    for (const auto& state : module.states) {
        if (state.initial) output << next++ << " init " << sorts.at(module.get(state.value).width)
                                  << ' ' << ids[state.value] << ' ' << ids[*state.initial] << '\n';
        output << next++ << " next " << sorts.at(module.get(state.value).width)
               << ' ' << ids[state.value] << ' ' << ids[*state.next] << '\n';
    }
    for (const auto& value : module.outputs) {
        output << next++ << " output " << ids.at(value.value);
        if (!value.name.empty()) output << ' ' << value.name;
        output << '\n';
    }
    for (const auto& property : module.properties) {
        if (property.kind == PropertyKind::justice) {
            output << next++ << " justice " << property.conditions.size();
            for (auto condition : property.conditions)
                output << ' ' << ids.at(condition);
        } else {
            auto opcode = property.kind == PropertyKind::constraint ? "constraint" :
                          property.kind == PropertyKind::bad ? "bad" :
                          property.kind == PropertyKind::fair ? "fair" : "cover";
            output << next++ << ' ' << opcode << ' ' << ids.at(property.condition);
        }
        if (!property.name.empty()) output << ' ' << property.name;
        output << '\n';
    }
}
}
