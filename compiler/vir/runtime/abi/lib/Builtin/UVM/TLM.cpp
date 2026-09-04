#include "Builtin/UVM/TLM.h"
#include <stdexcept>

namespace vir::runtime::builtin::uvm {
namespace {
std::string item(std::uint64_t index) { return "item." + std::to_string(index); }
std::uint64_t count(ObjectStore& objects, ObjectHandle fifo) {
    auto result = objects.load(fifo, "size");
    if (!result.ready()) throw std::runtime_error("invalid serialized UVM FIFO");
    return std::get<std::uint64_t>(result.value());
}
}
void TLM::connect(std::string port, std::string endpoint) {
    if (port.empty() || endpoint.empty())
        throw std::invalid_argument("empty UVM TLM endpoint");
    std::lock_guard lock(mutex_);
    connections_[std::move(port)].push_back(std::move(endpoint));
}
void TLM::subscribe(std::string endpoint, Subscriber callback) {
    if (endpoint.empty() || !callback)
        throw std::invalid_argument("invalid UVM TLM subscriber");
    std::lock_guard lock(mutex_);
    subscribers_[std::move(endpoint)].push_back(std::move(callback));
}
std::vector<std::string> TLM::write(std::string_view port, Value value) {
    std::vector<Subscriber> callbacks;
    std::vector<std::string> fifos;
    {
        std::lock_guard lock(mutex_);
        std::vector<std::string> pending{std::string(port)};
        std::unordered_map<std::string, bool> visited;
        while (!pending.empty()) {
            auto current = std::move(pending.back()); pending.pop_back();
            if (!visited.emplace(current, true).second) continue;
            auto links = connections_.find(current);
            if (links != connections_.end()) {
                pending.insert(pending.end(), links->second.begin(), links->second.end());
                continue;
            }
            if (current.ends_with("_fifo")) fifos.push_back(current);
            auto found = subscribers_.find(current);
            if (found != subscribers_.end())
                callbacks.insert(callbacks.end(), found->second.begin(), found->second.end());
        }
    }
    for (const auto& callback : callbacks) callback(value);
    for (const auto& fifo : fifos) fifo_push(fifo, value);
    return fifos;
}
void TLM::fifo_push(std::string_view name, Value value) {
    std::lock_guard lock(mutex_);
    auto& fifo = fifos_[std::string(name)];
    if (!fifo) {
        fifo = objects_.create("uvm_tlm_fifo");
        objects_.store(fifo, "name", std::string(name));
        objects_.store(fifo, "size", std::uint64_t{0});
    }
    auto size = count(objects_, fifo);
    objects_.store(fifo, item(size), std::move(value));
    objects_.store(fifo, "size", size + 1);
}
std::optional<Value> TLM::fifo_try_get(std::string_view name) {
    std::lock_guard lock(mutex_);
    auto found = fifos_.find(std::string(name));
    if (found == fifos_.end()) return {};
    auto size = count(objects_, found->second);
    if (!size) return {};
    auto result = objects_.load(found->second, item(0));
    if (!result.ready()) throw std::runtime_error("missing serialized UVM FIFO item");
    for (std::uint64_t index = 1; index < size; ++index) {
        auto next = objects_.load(found->second, item(index));
        if (!next.ready()) throw std::runtime_error("missing serialized UVM FIFO item");
        objects_.store(found->second, item(index - 1), next.value());
    }
    objects_.store(found->second, "size", size - 1);
    return result.value();
}
void TLM::reset() {
    std::lock_guard lock(mutex_);
    for (const auto& [_, fifo] : fifos_) objects_.destroy(fifo);
    connections_.clear();
    subscribers_.clear();
    fifos_.clear();
}
} // namespace vir::runtime::builtin::uvm
