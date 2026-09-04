#include "Builtin/Formal/Formal.h"

namespace vir::runtime::builtin {
void Formal::assert_property(std::string name, Property& property) { obligations_.push_back({Kind::assertion, std::move(name), &property}); }
void Formal::assume(std::string name, Property& property) { obligations_.push_back({Kind::assumption, std::move(name), &property}); }
void Formal::cover(std::string name, Property& property) { obligations_.push_back({Kind::cover, std::move(name), &property}); }
bool Formal::prove(const Prover& prover) const { for (auto& obligation : obligations_) if (prover ? !prover(obligation) : !obligation.property->evaluate()) return false; return true; }
}
