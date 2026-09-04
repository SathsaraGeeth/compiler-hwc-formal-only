#include "Transport/SignalAccess.h"

namespace vir::runtime {

transport::Status SignalAccess::export_signal(
    transport::Signal signal, const transport::Value& value) {
    if (!value.width || value.width > 64) return transport::Status::ERROR;
    return transport_.try_export_signal(signal, value);
}

transport::Status SignalAccess::import_signal(
    transport::Signal signal, transport::Value& value, bool peek) {
    const auto status = transport_.try_import_signal(signal, value, peek);
    if (status == transport::Status::SUCCESS &&
        (!value.width || value.width > 64))
        return transport::Status::ERROR;
    return status;
}

} // namespace vir::runtime
