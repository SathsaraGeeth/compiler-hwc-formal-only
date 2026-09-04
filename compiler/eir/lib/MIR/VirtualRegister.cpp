/*
 * compiler/eir/lib/MIR/VirtualRegister.cpp
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

#include "../../include/MIR/VirtualRegister.h"
#include <stdexcept>

namespace emul::mir {
VirtualRegister VirtualRegister::parse(std::string_view spelling) {
    if (spelling.size() < 2 || spelling.front() != '%')
        throw std::runtime_error(
            "invalid MIR virtual register " + std::string(spelling));
    return {std::string(spelling.substr(1))};
}

std::string VirtualRegister::spelling() const {
    return "%" + name;
}

bool VirtualRegister::valid() const noexcept {
    return !name.empty();
}
}
