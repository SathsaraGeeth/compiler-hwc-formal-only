/*
 * compiler/btor2/include/Lowering/Formal/temporary_file.h
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
 * Manages a securely created temporary transition system file
 */

#pragma once
#include <filesystem>

namespace emul::formal {
class TemporaryFile {
public:
    TemporaryFile();
    ~TemporaryFile();
    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};
}
