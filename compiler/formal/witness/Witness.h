#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace emul::formal {
struct WitnessSignal {
    uint64_t id = 0;
    std::string name;
    std::string bits;
    bool input = false;
};

struct WitnessStep {
    uint32_t index = 0;
    std::vector<WitnessSignal> signals;
};

struct Witness {
    std::vector<WitnessStep> steps;
    bool empty() const noexcept { return steps.empty(); }
};

Witness decode_btor_witness(const std::filesystem::path& model,
                            const std::string& text);
void write_witness_text(const Witness&, const std::filesystem::path&);
void write_witness_vcd(const Witness&, const std::filesystem::path&,
                       std::string_view timescale = "1ns");
}
