/*
 * compiler/btor2/lib/Conversion/HIRToIR.cpp
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

#include "Conversion/HIRToIR.h"
#include "Lowering/Formal/lower_sva.h"
#include "Lowering/Formal/writer.h"
#include <stdexcept>
#include <vector>

namespace emul::formal {
void convert_hir_to_ir(
    const btor2::hir::Module& module,
    std::string_view selected_property,
    PropertyTarget target,
    TransitionSystem& system) {
    const btor2::hir::Operation* selected = nullptr;
    const btor2::hir::Operation* only_property = nullptr;
    size_t property_count = 0;
    for (const auto& operation : module.operations) {
        const auto& directive = operation.directive;
        auto environment =
            directive.kind == frontend::sva::DirectiveKind::assume_property ||
            directive.kind == frontend::sva::DirectiveKind::restrict_property;
        if (directive.name == selected_property)
            selected = &operation;
        else if (environment)
            lower_sva_directive(directive, system);
        else {
            ++property_count;
            only_property = &operation;
        }
    }
    if (!selected && property_count == 1)
        selected = only_property;
    if (!selected)
        throw std::runtime_error(
            "named property not found: " + std::string(selected_property));

    const auto multiple = selected->components.size() > 1;
    for (const auto& component : selected->components) {
        const auto safety = component.opcode == btor2::hir::Opcode::safety;
        if ((target == PropertyTarget::safety && !safety) ||
            (target == PropertyTarget::liveness && safety))
            continue;

        auto part = selected->directive;
        part.property = component.formula;
        if (multiple)
            part.name += "." + std::string(
                btor2::hir::spelling(component.opcode));
        lower_sva_directive(part, system);
    }
}
} 
