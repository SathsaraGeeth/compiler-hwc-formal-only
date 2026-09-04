/*
 * Verifies the contract between target-independent EIR CodeGen and the
 * emulator target description. The test checks machine facts only; it does
 * not exercise lowering or optimization.
 */

#include "target/emul/emul_target.h"

int main() {
    const auto& target = emul::target::emulator::get_target();
    if (target.register_count() != 32 || target.first_allocatable_register() != 1)
        return 1;
    auto add = target.opcode("add");
    if (!add || *add != 0x03 || target.opcode("unknown")) return 2;
    auto word = target.encode(*add, 3, 4, 5, 0, {8, true}, 0);
    if (word >> 26 != 0x03 || ((word >> 21) & 31) != 3 ||
        ((word >> 16) & 31) != 4 || ((word >> 11) & 31) != 5 ||
        (word & 0x7ffu) != 0)
        return 3;
    const auto zext = target.opcode("zext");
    if (!zext || target.encode(*zext, 6, 7, 0, 0, {32, false}, 31) !=
                    (uint32_t{0x18} << 26 | 6u << 21 | 7u << 16 | 31u))
        return 4;
    const auto li = target.opcode("li");
    if (!li || target.encode(*li, 8, 0, 0, 0, {17, false}, 0x1234) !=
                  (uint32_t{0x33} << 26 | 8u << 21 | 16u << 16 | 0x1234u))
        return 5;
    return 0;
}
