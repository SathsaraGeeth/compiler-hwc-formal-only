/*
 * compiler/tools/src/words.cpp
 *
 * Copyright (C) 2026 Sathsara Geeth
 */

/*
 * Version 1.0
 *
 * Version History
 *
 * Version | Description
 * --------+-----------------------------------------
 * 1.0     | Initial implementation
 */

/*
 * Comments:
 *
 */

#include "words.h"
#include <cctype>
#include <stdexcept>

namespace emul::tool {
std::vector<std::string> split_words(std::string_view text) {
    std::vector<std::string> result;
    std::string word;
    char closing = 0;
    bool active = false;
    for (size_t index = 0; index < text.size(); ++index) {
        const char character = text[index];
        if (!closing && character == '#')
            break;
        if (!closing && std::isspace(static_cast<unsigned char>(character))) {
            if (active) {
                result.push_back(std::move(word));
                word.clear();
                active = false;
            }
            continue;
        }
        if (!closing && (character == '"' || character == '{')) {
            closing = character == '{' ? '}' : '"';
            active = true;
            continue;
        }
        if (closing && character == closing) {
            closing = 0;
            continue;
        }
        if (character == '\\' && index + 1 < text.size()) {
            word += text[++index];
            active = true;
            continue;
        }
        word += character;
        active = true;
    }
    if (closing)
        throw std::runtime_error("unterminated quoted word");
    if (active)
        result.push_back(std::move(word));
    return result;
}
}
