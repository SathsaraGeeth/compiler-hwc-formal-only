#pragma once
#include "VIR/Intrinsics.h"
#include "VIR/Operation.h"
namespace vir {
class IntrinsicInst {
public:
    explicit IntrinsicInst(Operation& operation) : operation_(operation) {}
    static bool classof(const Operation& operation);
    Intrinsic::ID intrinsic_id() const;
    Operation& operation() const { return operation_; }
private:
    Operation& operation_;
};
}
