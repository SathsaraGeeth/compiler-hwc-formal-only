#pragma once

#include "runtime/transport/transport.h"
#include <cstdint>
#include <string>
#include <variant>

namespace vir::runtime {

struct ObjectHandle {
    uint64_t value = 0;
    explicit operator bool() const noexcept { return value != 0; }
    friend bool operator==(ObjectHandle, ObjectHandle) = default;
};

using Value = std::variant<std::monostate, bool, int64_t, uint64_t,
                           std::string, transport::Value, ObjectHandle>;

} // namespace vir::runtime
