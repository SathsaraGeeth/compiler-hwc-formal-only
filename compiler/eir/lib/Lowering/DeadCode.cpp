/*
 * compiler/eir/lib/Lowering/DeadCode.cpp
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

#include "../../include/Lowering/DeadCode.h"
#include <algorithm>
#include <regex>
#include <unordered_set>

namespace emul::eir::lowering {
void eliminate_dead_code(Module& module) {
    std::unordered_set<std::string> live;
    std::vector<Operation> kept;
    kept.reserve(module.operations.size());

    for (auto operation = module.operations.rbegin();
         operation != module.operations.rend(); ++operation) {
        auto results = operation->result_list();
        auto required = operation->has_side_effects() ||
                        operation->opcode == "instance";
        for (const auto& result : results)
            required |= live.contains(result);
        if (!required)
            continue;
        for (const auto& result : results)
            live.erase(result);
        static const std::regex value(R"(%[A-Za-z_0-9]+)");
        for (std::sregex_iterator operand(
                 operation->operands.begin(), operation->operands.end(), value),
             end; operand != end; ++operand)
            live.insert(operand->str());
        kept.push_back(*operation);
    }
    std::reverse(kept.begin(), kept.end());
    module.operations = std::move(kept);
}
}
