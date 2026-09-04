/*
 * compiler/eir/include/MIR/MachineProgram.h
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
 * 1. Final target program produced from MIR.
 * 2. Binding connects a named EIR value to target storage.
 * 3. Attrs: Binding
 *    - name
 *    - kind; import, export, or persistent state
 *    - address
 *    - width
 *    - four_state
 * 4. Attrs: Program
 *    - words; encoded target instructions
 *    - bindings; target-visible values and states
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace emul::machine {
enum class BindingKind {
    import,
    export_value,
    state
};

struct Binding {
    std::string name;
    BindingKind kind{};
    uint32_t address = 0;
    uint32_t width = 0;
    bool four_state = true;
};

struct Program {
    std::vector<uint64_t> words;
    std::vector<Binding> bindings;
};
}
