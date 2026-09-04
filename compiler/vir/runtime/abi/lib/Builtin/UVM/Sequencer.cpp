#include "Builtin/UVM/Sequencer.h"

namespace vir::runtime::builtin::uvm {
std::shared_ptr<Channel> Sequencer::channel(std::string_view name) {
    std::lock_guard lock(mutex_);
    auto& selected = channels_[std::string(name)];
    if (!selected) selected = std::make_shared<Channel>();
    return selected;
}
void Sequencer::start_item(std::string sequencer, Value item) {
    channel(sequencer)->send(std::move(item));
}
Value Sequencer::get_next_item(std::string_view sequencer) {
    return channel(sequencer)->receive();
}
std::optional<Value> Sequencer::try_next_item(std::string_view sequencer) {
    return channel(sequencer)->try_receive();
}
void Sequencer::reset() { std::lock_guard lock(mutex_); channels_.clear(); }
} // namespace vir::runtime::builtin::uvm
