#pragma once
#include "../Property/Property.h"
#include <functional>
#include <string>
#include <vector>

namespace vir::runtime::builtin {
class Formal {
public:
    enum class Kind { assertion, assumption, cover };
    struct Obligation { Kind kind; std::string name; Property* property; };
    using Prover = std::function<bool(const Obligation&)>;
    void assert_property(std::string name, Property& property);
    void assume(std::string name, Property& property);
    void cover(std::string name, Property& property);
    bool prove(const Prover& prover = {}) const;
    const std::vector<Obligation>& obligations() const { return obligations_; }
private:
    std::vector<Obligation> obligations_;
};
}
