#include "vir_/runtime/vir_runtime/executor/executor.h"
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {
class MemoryTransport final : public emul::runtime::Transport {
public:
    void drive(std::string_view signal,
               const emul::runtime::Value& value) override {
        values_[std::string(signal)] = value;
    }

    emul::runtime::Value sample(std::string_view signal,
                                uint32_t width) override {
        auto found = values_.find(std::string(signal));
        return found == values_.end()
            ? emul::runtime::Value::known(width, 0) : found->second;
    }

    void advance(uint64_t) override {}
    void transfer(std::span<const std::byte>, std::span<std::byte>) override {}

    uint64_t scalar(std::string_view signal) const {
        return values_.at(std::string(signal)).bits.at(0);
    }

private:
    std::unordered_map<std::string, emul::runtime::Value> values_;
};
}

int main() {
    emul::vir::Process process{"loop", {
        {"%zero", "const", "0"},
        {{}, "store", "@count, %zero"},
        {{}, "label", "test"},
        {"%count", "load", "@count"},
        {"%limit", "const", "3"},
        {"%more", "binary", "LessThan, %count, %limit"},
        {{}, "condbr", "%more, body, end"},
        {{}, "label", "body"},
        {"%current", "load", "@count"},
        {"%one", "const", "1"},
        {"%next", "binary", "Add, %current, %one"},
        {{}, "store", "@count, %next"},
        {{}, "br", "test"},
        {{}, "label", "end"},
        {{}, "return", "0"}
    }};
    MemoryTransport transport;
    emul::runtime::Executor executor(transport);
    emul::vir::Program program{{process}};
    if (!executor.run(program)) return 1;
    return transport.scalar("count") == 3 ? 0 : 2;
}
