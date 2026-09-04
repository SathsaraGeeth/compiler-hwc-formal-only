#include "Builtin/Port/Port.h"
#include <stdexcept>

namespace vir::runtime::builtin {
void Port::connect(Port& port) { if (!channel_ && !port.channel_) channel_ = port.channel_ = std::make_shared<Channel>(); else if (channel_) port.channel_ = channel_; else channel_ = port.channel_; }
void Port::send(Value transaction) { if (!channel_) throw std::runtime_error("unconnected port"); channel_->send(std::move(transaction)); }
Value Port::receive() { if (!channel_) throw std::runtime_error("unconnected port"); return channel_->receive(); }
}
