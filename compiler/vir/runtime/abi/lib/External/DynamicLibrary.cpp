#include "External/DynamicLibrary.h"
#include <dlfcn.h>
#include <utility>

namespace vir::runtime {

DynamicLibrary::DynamicLibrary(std::string path) {
    handle_ = ::dlopen(path.empty() ? nullptr : path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle_) error_ = ::dlerror();
}

DynamicLibrary::~DynamicLibrary() {
    if (handle_) ::dlclose(handle_);
}

DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)), error_(std::move(other.error_)) {}

DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
    if (this == &other) return *this;
    if (handle_) ::dlclose(handle_);
    handle_ = std::exchange(other.handle_, nullptr);
    error_ = std::move(other.error_);
    return *this;
}

bool DynamicLibrary::import(ExternalRegistry& registry, std::string name,
                            std::string_view symbol) const {
    if (!handle_ || symbol.empty()) return false;
    ::dlerror();
    auto* address = ::dlsym(handle_, std::string(symbol).c_str());
    if (::dlerror() || !address) return false;
    auto function = reinterpret_cast<NativeFunction>(address);
    return registry.add(std::move(name),
                        [function](std::span<const Value> args) { return function(args); });
}

} // namespace vir::runtime
