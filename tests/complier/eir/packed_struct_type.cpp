#include "Lowering/Frontend/context.h"

int main() {
    using emul::lowering::semantic::lower_type;
    if (lower_type("struct packed{logic valid;logic [7:0] addr;}") !=
        "4s<9>")
        return 1;
    if (lower_type("struct packed{bit valid;bit [7:0] addr;}") !=
        "2s<9>")
        return 2;
    return 0;
}
