#pragma once

#include "../Core/Result.h"
#include "runtime/transport/transport.h"

namespace vir::runtime {

class SignalAccess {
public:
    explicit SignalAccess(transport::Transport& transport)
        : transport_(transport) {}
    transport::Status export_signal(transport::Signal signal,
                                    const transport::Value& value);
    transport::Status import_signal(transport::Signal signal,
                                    transport::Value& value,
                                    bool peek = false);
private:
    transport::Transport& transport_;
};

} // namespace vir::runtime
