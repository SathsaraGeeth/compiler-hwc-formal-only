#pragma once

#include "runtime/transport/transport.h"
#include <string>

namespace vir::runtime::abi {

void install_transport(transport::Transport& transport) noexcept;
void remove_transport() noexcept;
void clear_design() noexcept;
void set_combinational(void (*callback)()) noexcept;
void set_clocked(void (*callback)()) noexcept;
bool register_signal(void* storage, transport::Signal signal,
                     std::uint32_t width, bool input);
bool register_clock(void* storage);
transport::Status tick();
void flush_nba();
void set_instance(std::string instance);
transport::Status synchronize();

} // namespace vir::runtime::abi
