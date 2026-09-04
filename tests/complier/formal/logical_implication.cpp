#include "Lowering/Formal/lower_expression.h"
#include <sstream>
#include <string>

int main() {
    using emul::frontend::sva::Expression;
    using emul::frontend::sva::ExpressionKind;

    emul::formal::TransitionSystem system;
    system.signals["antecedent"] = system.builder.input(1, "antecedent");
    system.signals["consequent"] = system.builder.input(1, "consequent");

    Expression implication{
        ExpressionKind::binary,
        "LogicalImplication",
        {},
        1,
        {
            {ExpressionKind::signal, {}, "antecedent", 1, {}},
            {ExpressionKind::signal, {}, "consequent", 1, {}}
        }
    };
    auto result = emul::formal::lower_sva_expression(implication, system);
    if (result.width != 1)
        return 1;

    std::ostringstream output;
    system.builder.write(output);
    auto text = output.str();
    return text.find(" not ") != std::string::npos &&
           text.find(" or ") != std::string::npos ? 0 : 2;
}
