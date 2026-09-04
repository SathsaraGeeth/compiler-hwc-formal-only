#pragma once

#include "../Core/Result.h"
#include "../Core/Value.h"
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>

namespace vir::runtime {

struct ClassDescriptor {
    using NativeObject = std::shared_ptr<void>;
    using Constructor = std::function<Result<NativeObject>(std::span<const Value>)>;
    using Method = std::function<Result<Value>(const NativeObject&,
                                                std::span<const Value>)>;
    std::string name;
    Constructor construct;
    std::unordered_map<std::string, Method> methods;
};

template<class T>
std::shared_ptr<T> native_cast(const ClassDescriptor::NativeObject& object) {
    return std::static_pointer_cast<T>(object);
}

} // namespace vir::runtime
