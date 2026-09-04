#include "Lowering/Formal/history.h"
#include <sstream>
#include <string>

int main() {
    emul::formal::TransitionSystem system;
    auto value = system.builder.input(8, "value");
    auto sample = system.builder.input(1, "sample");
    auto history = emul::formal::SampledHistory::previous(
        value, 3, system, sample);
    system.builder.output(value, "value_out");
    system.builder.constraint(sample, "sample_constraint");
    system.builder.bad(sample, "sample_bad");
    system.builder.fair(sample, "sample_fair");
    system.builder.justice({sample}, "sample_justice");
    if (history.width != 8 || system.next_auxiliary != 3)
        return 1;
    std::ostringstream output;
    system.builder.write(output);
    auto text = output.str();
    size_t states = 0;
    size_t nexts = 0;
    for (size_t position = 0;
         (position = text.find(" state ", position)) != std::string::npos;
         ++position)
        ++states;
    for (size_t position = 0;
         (position = text.find(" next ", position)) != std::string::npos;
         ++position)
        ++nexts;
    auto statements = text.find(" output ") != std::string::npos &&
        text.find(" constraint ") != std::string::npos &&
        text.find(" bad ") != std::string::npos &&
        text.find(" fair ") != std::string::npos &&
        text.find(" justice ") != std::string::npos;
    return states == 3 && nexts == 3 && statements ? 0 : 2;
}
