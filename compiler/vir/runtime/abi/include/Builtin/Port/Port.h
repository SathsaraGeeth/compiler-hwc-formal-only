#pragma once
#include "../Channel/Channel.h"
#include <memory>

namespace vir::runtime::builtin {
class Port {
public:
    Port() = default;
    void connect(Port& port);
    bool connected() const noexcept { return channel_ != nullptr; }
    void send(Value transaction);
    Value receive();
private:
    std::shared_ptr<Channel> channel_;
};
}
