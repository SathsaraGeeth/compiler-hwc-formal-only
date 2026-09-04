/*
 * compiler/eir/lib/CodeGen/Emit/ProgramWriter.cpp
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

#include "../../../include/CodeGen/Emit/ProgramWriter.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace emul::executable {
namespace {
constexpr uint64_t align(uint64_t value) { return (value + 7) & ~uint64_t{7}; }
template<class T> void put(std::vector<std::byte>& output, size_t offset, T value) {
    std::memcpy(output.data() + offset, &value, sizeof(value));
}
struct Section { uint32_t type, flags; uint64_t offset, size; uint32_t entry_size, count; };
}

void write(const std::filesystem::path& path, const machine::Program& machine,
           const vir::Program& host_program) {
    std::string strings(1, '\0');
    std::unordered_map<std::string, uint32_t> offsets;
    for (const auto& binding : machine.bindings) {
        if (!binding.width || binding.width > 65535)
            throw std::runtime_error("binding width is not representable");
        if (offsets.contains(binding.name)) continue;
        offsets[binding.name] = static_cast<uint32_t>(strings.size());
        strings += binding.name;
        strings += '\0';
    }
    std::ostringstream vir_text;
    vir::print(host_program, vir_text);
    auto host = vir_text.str();
    constexpr uint64_t header_size = 64, section_size = 32, section_count = 4;
    uint64_t cursor = header_size + section_size * section_count;
    std::vector<Section> sections;
    auto add = [&](uint32_t type, uint64_t bytes, uint32_t entry, uint32_t count) {
        cursor = align(cursor);
        sections.push_back({type, 0, cursor, bytes, entry, count});
        cursor += bytes;
    };
    add(1, strings.size(), 1, strings.size());
    add(2, machine.words.size() * 8, 8, machine.words.size());
    add(3, machine.bindings.size() * 16, 16, machine.bindings.size());
    add(4, host.size(), 1, host.size());
    std::vector<std::byte> output(align(cursor));
    const unsigned char magic[8]{0x7f, 'E', 'I', 'R', 'C', 'E', 'X', 'E'};
    std::memcpy(output.data(), magic, sizeof(magic));
    put<uint16_t>(output, 8, 1); put<uint16_t>(output, 10, 0);
    put<uint16_t>(output, 12, header_size); put<uint16_t>(output, 14, section_size);
    put<uint16_t>(output, 16, section_count); put<uint64_t>(output, 24, header_size);
    put<uint64_t>(output, 32, output.size());
    for (size_t index = 0; index < sections.size(); ++index) {
        auto base = header_size + index * section_size;
        put(output, base, sections[index].type); put(output, base + 4, sections[index].flags);
        put(output, base + 8, sections[index].offset); put(output, base + 16, sections[index].size);
        put(output, base + 24, sections[index].entry_size);
        put(output, base + 28, sections[index].count);
    }
    std::memcpy(output.data() + sections[0].offset, strings.data(), strings.size());
    std::memcpy(output.data() + sections[1].offset, machine.words.data(), sections[1].size);
    for (size_t index = 0; index < machine.bindings.size(); ++index) {
        auto base = sections[2].offset + index * 16;
        const auto& binding = machine.bindings[index];
        put(output, base, offsets.at(binding.name)); put(output, base + 4, binding.address);
        put<uint16_t>(output, base + 8, binding.width);
        auto kind = binding.kind == machine::BindingKind::import ? 1 :
                    binding.kind == machine::BindingKind::export_value ? 2 : 3;
        put<uint8_t>(output, base + 10, kind); put<uint8_t>(output, base + 11, binding.four_state);
    }
    std::memcpy(output.data() + sections[3].offset, host.data(), host.size());
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot write " + path.string());
    file.write(reinterpret_cast<const char*>(output.data()), output.size());
}
}
