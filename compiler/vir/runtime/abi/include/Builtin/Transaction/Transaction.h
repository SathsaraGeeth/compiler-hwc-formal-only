#pragma once
#include "../../Core/Result.h"
#include "../../Core/Value.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace vir::runtime::builtin {
class Transaction {
public:
    explicit Transaction(std::string type) : type_(std::move(type)) {}
    void set(std::string field, Value value);
    Result<Value> get(std::string_view field) const;
    Transaction clone() const { return *this; }
    bool compare(const Transaction& other) const;
    const std::string& type() const { return type_; }
private:
    std::string type_;
    std::unordered_map<std::string, Value> fields_;
};
}
