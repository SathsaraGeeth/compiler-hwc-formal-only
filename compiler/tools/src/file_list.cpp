/*
 * compiler/tools/src/file_list.cpp
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

#include "file_list.h"
#include "words.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace emul::tool {
namespace {
std::filesystem::path resolve(
    const std::filesystem::path& base,
    const std::filesystem::path& path) {
    return std::filesystem::absolute(path.is_absolute() ? path : base / path)
        .lexically_normal();
}

template<class Value>
void append_unique(std::vector<Value>& values, Value value) {
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(std::move(value));
}

void read(Session& session, const std::filesystem::path& path,
          std::unordered_set<std::string>& active) {
    const auto file = std::filesystem::absolute(path).lexically_normal();
    if (!active.insert(file.string()).second)
        throw std::runtime_error("recursive file list: " + file.string());
    std::ifstream input(file);
    if (!input)
        throw std::runtime_error("cannot read file list: " + file.string());

    const auto base = file.parent_path();
    std::string line;
    size_t line_number = 0;
    try {
        while (std::getline(input, line)) {
            ++line_number;
            auto words = split_words(line);
            for (size_t index = 0; index < words.size(); ++index) {
                const auto& word = words[index];
                if (word == "-f") {
                    if (++index == words.size())
                        throw std::runtime_error("-f needs a file name");
                    read(session, resolve(base, words[index]), active);
                } else if (word == "-I") {
                    if (++index == words.size())
                        throw std::runtime_error("-I needs a directory");
                    append_unique(session.inputs.include_dirs,
                                  resolve(base, words[index]));
                } else if (word == "-D") {
                    if (++index == words.size())
                        throw std::runtime_error("-D needs a definition");
                    append_unique(session.inputs.defines, words[index]);
                } else if (word == "-v") {
                    if (++index == words.size())
                        throw std::runtime_error("-v needs a source file");
                    append_unique(session.inputs.libraries,
                                  resolve(base, words[index]));
                } else if (word.starts_with("+incdir+")) {
                    size_t start = 8;
                    while (start <= word.size()) {
                        const auto end = word.find('+', start);
                        const auto directory = word.substr(start, end - start);
                        if (!directory.empty())
                            append_unique(session.inputs.include_dirs,
                                          resolve(base, directory));
                        if (end == std::string::npos) break;
                        start = end + 1;
                    }
                } else if (word.starts_with("+define+")) {
                    size_t start = 8;
                    while (start <= word.size()) {
                        const auto end = word.find('+', start);
                        const auto definition = word.substr(start, end - start);
                        if (!definition.empty())
                            append_unique(session.inputs.defines, definition);
                        if (end == std::string::npos) break;
                        start = end + 1;
                    }
                } else if (!word.empty() && word.front() == '-') {
                    throw std::runtime_error("unsupported file-list option: " + word);
                } else {
                    append_unique(session.inputs.sources, resolve(base, word));
                }
            }
        }
    } catch (const std::exception& error) {
        active.erase(file.string());
        throw std::runtime_error(file.string() + ':' +
                                 std::to_string(line_number) + ": " + error.what());
    }
    active.erase(file.string());
}
}

void read_file_list(Session& session, const std::filesystem::path& path) {
    std::unordered_set<std::string> active;
    read(session, path, active);
}
}
