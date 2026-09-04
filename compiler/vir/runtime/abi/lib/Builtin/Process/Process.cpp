#include "Builtin/Process/Process.h"

namespace vir::runtime::builtin {
Process::Process(Function function) : function_(std::move(function)) {}
Process::~Process() { kill(); }
void Process::spawn() {
    State expected = State::created;
    if (!state_.compare_exchange_strong(expected, State::running)) return;
    thread_ = std::jthread([this](std::stop_token stop) {
        try { function_(stop); state_ = stop.stop_requested() ? State::stopped : State::completed; }
        catch (...) { state_ = State::failed; }
    });
}
void Process::join() { if (thread_.joinable()) thread_.join(); }
void Process::kill() { if (thread_.joinable()) { thread_.request_stop(); thread_.join(); } }
}
