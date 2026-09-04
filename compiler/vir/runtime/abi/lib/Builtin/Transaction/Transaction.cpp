#include "Builtin/Transaction/Transaction.h"

namespace vir::runtime::builtin {
void Transaction::set(std::string field, Value value) { fields_.insert_or_assign(std::move(field), std::move(value)); }
Result<Value> Transaction::get(std::string_view field) const { auto found = fields_.find(std::string(field)); return found == fields_.end() ? Result<Value>::failed("unknown transaction field") : Result<Value>::completed(found->second); }
bool Transaction::compare(const Transaction& other) const { return type_ == other.type_ && fields_ == other.fields_; }
}
