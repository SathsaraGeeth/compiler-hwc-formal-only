#pragma once
#include "../../Object/ObjectStore.h"
#include <string>

namespace vir::runtime::builtin {
class Object {
public:
    explicit Object(ObjectStore& store) : store_(store) {}
    ObjectHandle create(std::string type) { return store_.create(std::move(type)); }
    Result<ObjectHandle> clone(ObjectHandle source);
    Result<bool> compare(ObjectHandle left, ObjectHandle right) const;
    Result<std::string> print(ObjectHandle object) const;
private:
    ObjectStore& store_;
};
}
