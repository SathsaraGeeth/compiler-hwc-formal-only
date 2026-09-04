#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace vir::runtime::io {

class EndOfInput final : public std::runtime_error {
public:
    EndOfInput() : std::runtime_error("end of VIR input") {}
};

class DescriptorIO {
public:
    std::uint64_t read(int descriptor, std::string_view name,
                       std::uint32_t width) const;
    void write(int descriptor, std::string_view name,
               std::uint64_t value, std::uint64_t xmask,
               std::uint64_t zmask, std::uint32_t width) const;
};

} // namespace vir::runtime::io
