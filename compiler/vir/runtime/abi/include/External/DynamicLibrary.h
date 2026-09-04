#pragma once

#include "Registry.h"
#include <string>
#include <string_view>

namespace vir::runtime {

class DynamicLibrary {
public:
    using NativeFunction = Result<Value> (*)(std::span<const Value>);

    explicit DynamicLibrary(std::string path);
    ~DynamicLibrary();
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

    bool valid() const noexcept { return handle_ != nullptr; }
    const std::string& error() const noexcept { return error_; }
    bool import(ExternalRegistry& registry, std::string name,
                std::string_view symbol) const;
private:
    void* handle_ = nullptr;
    std::string error_;
};

} // namespace vir::runtime
