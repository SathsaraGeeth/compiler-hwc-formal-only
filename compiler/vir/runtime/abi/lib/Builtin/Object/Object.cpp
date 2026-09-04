#include "Builtin/Object/Object.h"

namespace vir::runtime::builtin {
Result<ObjectHandle> Object::clone(ObjectHandle source) {
    return store_.clone(source);
}
Result<bool> Object::compare(ObjectHandle left, ObjectHandle right) const {
    return store_.compare(left, right);
}
Result<std::string> Object::print(ObjectHandle object) const {
    auto type = store_.type(object);
    if (!type.ready()) return Result<std::string>::failed(type.error());
    return Result<std::string>::completed(type.value() + "#" + std::to_string(object.value));
}
}
