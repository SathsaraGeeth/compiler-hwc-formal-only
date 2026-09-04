#include "IO/DescriptorIO.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

namespace vir::runtime::io {
namespace {

void write_all(int descriptor, std::string_view text) {
    while (!text.empty()) {
        auto count = ::write(descriptor, text.data(), text.size());
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0)
            throw std::system_error(errno, std::generic_category(),
                                    "VIR descriptor write");
        text.remove_prefix(static_cast<std::size_t>(count));
    }
}

bool read_line(int descriptor, std::string& output) {
    output.clear();
    char byte = 0;
    for (;;) {
        auto count = ::read(descriptor, &byte, 1);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0)
            throw std::system_error(errno, std::generic_category(),
                                    "VIR descriptor read");
        if (!count) return !output.empty();
        if (byte == '\n') return true;
        if (byte != '\r') output.push_back(byte);
    }
}

std::uint64_t width_mask(std::uint32_t width) {
    return width == 64 ? ~std::uint64_t{} : (std::uint64_t{1} << width) - 1;
}

} // namespace

std::uint64_t DescriptorIO::read(int descriptor, std::string_view name,
                                 std::uint32_t width) const {
    if (!width || width > 64) throw std::runtime_error("invalid VIR I/O width");
    if (::isatty(descriptor)) write_all(STDOUT_FILENO, std::string(name) + "> ");
    std::string text;
    if (!read_line(descriptor, text)) throw EndOfInput();
    text.erase(std::remove_if(text.begin(), text.end(),
        [](unsigned char byte) { return std::isspace(byte); }), text.end());
    int base = 10;
    std::string_view digits = text;
    if (digits.starts_with("0x") || digits.starts_with("0X")) {
        base = 16;
        digits.remove_prefix(2);
    } else if (digits.starts_with("0b") || digits.starts_with("0B")) {
        base = 2;
        digits.remove_prefix(2);
    }
    std::uint64_t value = 0;
    auto [end, error] = std::from_chars(
        digits.data(), digits.data() + digits.size(), value, base);
    if (error != std::errc{} || end != digits.data() + digits.size())
        throw std::runtime_error("invalid value for " + std::string(name) + ": " + text);
    return value & width_mask(width);
}

void DescriptorIO::write(int descriptor, std::string_view name,
                         std::uint64_t value, std::uint64_t xmask,
                         std::uint64_t zmask, std::uint32_t width) const {
    if (!width || width > 64) throw std::runtime_error("invalid VIR I/O width");
    static constexpr char hex[] = "0123456789abcdef";
    std::string text(name);
    text += "=0x";
    auto digits = std::max(1u, (width + 3u) / 4u);
    for (unsigned index = digits; index-- > 0;)
        text.push_back(hex[(value >> (index * 4)) & 15]);
    text += " xmask=0x";
    for (unsigned index = digits; index-- > 0;)
        text.push_back(hex[(xmask >> (index * 4)) & 15]);
    text += " zmask=0x";
    for (unsigned index = digits; index-- > 0;)
        text.push_back(hex[(zmask >> (index * 4)) & 15]);
    text += " width=" + std::to_string(width) + "\n";
    write_all(descriptor, text);
}

} // namespace vir::runtime::io
