#pragma once

#include "Builtin/Cover/Cover.h"
#include <mutex>
#include <iosfwd>
#include <string>
#include <unordered_set>

namespace vir::runtime::builtin::uvm {

class Coverage {
public:
    void register_bin(std::string bin);
    void sample(std::string bin, const Value& value);
    double percentage() const;
    void report(std::ostream& output) const;
    void reset();

private:
    mutable std::mutex mutex_;
    Cover cover_;
    std::unordered_set<std::string> bins_;
};

} // namespace vir::runtime::builtin::uvm
