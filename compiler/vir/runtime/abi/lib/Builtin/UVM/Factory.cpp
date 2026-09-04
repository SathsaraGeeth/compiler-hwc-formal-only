#include "Builtin/UVM/Factory.h"
#include <mutex>

namespace vir::runtime::builtin::uvm {
void Factory::set_override(std::string requested, std::string replacement) {
    std::unique_lock lock(mutex_);
    overrides_.insert_or_assign(std::move(requested), std::move(replacement));
}
Result<ObjectHandle> Factory::create(std::string_view type, std::string name,
                                     ObjectHandle parent) {
    std::string selected(type);
    {
        std::shared_lock lock(mutex_);
        if (auto found = overrides_.find(selected); found != overrides_.end())
            selected = found->second;
    }
    auto object = objects_.create(selected);
    if (!object) return Result<ObjectHandle>::failed("cannot create UVM type " + selected);
    objects_.store(object, "name", std::move(name));
    objects_.store(object, "parent", parent);
    return Result<ObjectHandle>::completed(object);
}
void Factory::reset() { std::unique_lock lock(mutex_); overrides_.clear(); }
} // namespace vir::runtime::builtin::uvm
