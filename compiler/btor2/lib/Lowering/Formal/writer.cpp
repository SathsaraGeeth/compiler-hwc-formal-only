/*
 * compiler/btor2/lib/Lowering/Formal/writer.cpp
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

#include "Lowering/Formal/writer.h"
#include "Lowering/Formal/harness.h"
#include "Conversion/HIRToIR.h"
#include "Conversion/SVAToHIR.h"
#include "Lowering/TwoStateEIRLowering.h"
#include "Transform/HIR/PassPipeline.h"
#include "Transform/IR/PassPipeline.h"
#include "Transform/IR/PropertyPartition.h"
#include "IR/Printer.h"
#include "IR/Verifier.h"
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace emul::formal {

btor2::Module build_transition_system(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    bool run_optimizations) {
    auto harness = build_harness(design, top, property);
    auto system = btor2::lower_two_state(harness.hardware);
    std::unordered_map<std::string, Btor2Value> unique_leaves;
    std::unordered_set<std::string> ambiguous_leaves;
    for (const auto& [name, value] : system.signals) {
        auto separator = name.rfind('.');
        if (separator == std::string::npos)
            continue;
        auto leaf = name.substr(separator + 1);
        if (!unique_leaves.emplace(leaf, value).second)
            ambiguous_leaves.insert(std::move(leaf));
    }
    for (const auto& [leaf, value] : unique_leaves)
        if (!ambiguous_leaves.contains(leaf))
            system.signals[leaf] = value;
    for (const auto& [host, hardware] : harness.signal_aliases) {
        auto signal = system.signals.find(hardware);
        if (signal != system.signals.end())
            system.signals[host] = signal->second;
    }
    for (const auto& [instance, module] : harness.instance_aliases) {
        std::vector<std::pair<std::string, Btor2Value>> aliases;
        for (const auto& [name, value] : system.signals)
            if (name.starts_with(module + "."))
                aliases.push_back({
                    instance + name.substr(module.size()), value});
        for (auto& [name, value] : aliases)
            system.signals[std::move(name)] = value;
    }
    for (const auto& flag : harness.host_flags) {
        auto clock = system.signals.find(flag.clock);
        if (clock == system.signals.end())
            throw std::runtime_error(
                "formal host clock not found: " + flag.clock);
        auto zero = system.builder.constant(1, "0");
        auto one = system.builder.constant(1, "1");
        auto previous = system.builder.state(
            1, flag.name + ".clock_previous");
        auto initialized = system.signals.at("formal_initialized");
        auto initial_clock = system.builder.binary(
            "eq", previous, clock->second, 1);
        system.builder.constraint(
            system.builder.binary("or", initialized, initial_clock, 1),
            flag.name + ".initial_clock");
        system.builder.next(previous, clock->second);
        auto edge = system.builder.binary(
            "and",
            system.builder.unary("not", previous, 1),
            clock->second, 1);
        auto state = system.builder.state(1, flag.name);
        system.builder.init(state, zero);
        system.builder.next(
            state,
            system.builder.ternary("ite", edge, one, state));
        system.signals[flag.name] = state;
    }
    auto high_level = btor2::convert_sva_to_hir(harness.directives);
    if (run_optimizations) btor2::transform::optimize(high_level);
    convert_hir_to_ir(high_level, property, PropertyTarget::all, system);
    if (run_optimizations) btor2::transform::optimize(system.builder.module());
    return system.builder.module();
}

void write_transition_system(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    std::ostream& output,
    PropertyTarget target,
    bool run_optimizations) {
    auto module = build_transition_system(
        design, top, property, run_optimizations);
    if (target == PropertyTarget::safety)
        module = btor2::transform::select_properties(
            module, btor2::transform::PropertyPartition::safety);
    else if (target == PropertyTarget::liveness)
        module = btor2::transform::select_properties(
            module, btor2::transform::PropertyPartition::liveness);
    btor2::verify(module);
    btor2::print(module, output);
}

void write_transition_system(
    const frontend::ElaboratedDesign& design,
    std::string_view top,
    std::string_view property,
    const std::filesystem::path& output) {
    std::ofstream stream(output);
    if (!stream)
        throw std::runtime_error(
            "cannot write transition system: " + output.string());
    write_transition_system(design, top, property, stream);
}
}
