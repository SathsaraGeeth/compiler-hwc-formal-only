/*
 * compiler/eir/lib/CodeGen/Emit/ProgramReader.cpp
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

#include "../../../include/CodeGen/Emit/ProgramReader.h"
#include "../../../../vir_/parser.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace emul::executable {
namespace {
template<class T> T get(const std::vector<std::byte>& input, size_t offset) {
    if (offset + sizeof(T) > input.size()) throw std::runtime_error("truncated EIRC executable");
    T value; std::memcpy(&value, input.data() + offset, sizeof(value)); return value;
}
struct Section { uint32_t type; uint64_t offset, size; uint32_t entry_size, count; };
}

Program read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot read " + path.string());
    auto size = file.tellg(); file.seekg(0);
    std::vector<std::byte> bytes(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    const unsigned char magic[8]{0x7f, 'E', 'I', 'R', 'C', 'E', 'X', 'E'};
    if (bytes.size() < 64 || std::memcmp(bytes.data(), magic, 8))
        throw std::runtime_error("invalid EIRC executable");
    if (get<uint16_t>(bytes, 8) != 1 || get<uint16_t>(bytes, 12) != 64 ||
        get<uint16_t>(bytes, 14) != 32 || get<uint64_t>(bytes, 32) != bytes.size())
        throw std::runtime_error("unsupported EIRC executable header");
    auto section_count = get<uint16_t>(bytes, 16);
    auto table = get<uint64_t>(bytes, 24);
    if (section_count != 4 || table > bytes.size() ||
        uint64_t(section_count) * 32 > bytes.size() - table)
        throw std::runtime_error("invalid EIRC section table");
    std::vector<Section> sections;
    for (size_t index = 0; index < section_count; ++index) {
        auto base = table + index * 32;
        Section section{get<uint32_t>(bytes, base), get<uint64_t>(bytes, base + 8),
                        get<uint64_t>(bytes, base + 16), get<uint32_t>(bytes, base + 24),
                        get<uint32_t>(bytes, base + 28)};
        if (section.offset > bytes.size() || section.size > bytes.size() - section.offset)
            throw std::runtime_error("invalid section bounds");
        sections.push_back(section);
    }
    auto find = [&](uint32_t type) -> const Section& {
        for (const auto& section : sections) if (section.type == type) return section;
        throw std::runtime_error("missing EIRC executable section");
    };
    const auto& string_section = find(1);
    if (string_section.entry_size != 1 || !string_section.count ||
        string_section.count != string_section.size)
        throw std::runtime_error("invalid string section");
    std::string strings(reinterpret_cast<const char*>(bytes.data() + string_section.offset),
                        string_section.size);
    Program result;
    const auto& code = find(2);
    if (code.entry_size != 8 || uint64_t(code.count) * 8 != code.size)
        throw std::runtime_error("invalid ISA section");
    result.machine.words.resize(code.count);
    std::memcpy(result.machine.words.data(), bytes.data() + code.offset, code.size);
    const auto& bindings = find(3);
    if (bindings.entry_size != 16 || uint64_t(bindings.count) * 16 != bindings.size)
        throw std::runtime_error("invalid binding section");
    for (size_t index = 0; index < bindings.count; ++index) {
        auto base = bindings.offset + index * 16;
        auto name_offset = get<uint32_t>(bytes, base);
        if (name_offset >= strings.size() ||
            strings.find('\0', name_offset) == std::string::npos)
            throw std::runtime_error("invalid binding name");
        auto kind = get<uint8_t>(bytes, base + 10);
        if (kind < 1 || kind > 3 || get<uint16_t>(bytes, base + 8) == 0)
            throw std::runtime_error("invalid binding record");
        result.machine.bindings.push_back({strings.c_str() + name_offset,
            kind == 1 ? machine::BindingKind::import
            : kind == 2 ? machine::BindingKind::export_value
                        : machine::BindingKind::state,
            get<uint32_t>(bytes, base + 4), get<uint16_t>(bytes, base + 8),
            bool(get<uint8_t>(bytes, base + 11))});
    }
    const auto& host = find(4);
    if (host.entry_size != 1 || host.count != host.size)
        throw std::runtime_error("invalid VIR section");
    std::istringstream host_input(std::string(
        reinterpret_cast<const char*>(bytes.data() + host.offset), host.size));
    result.host = vir::parse(host_input);
    return result;
}
}
