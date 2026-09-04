/*
 * compiler/btor2/lib/Lowering/Formal/temporary_file.cpp
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

#include "Lowering/Formal/temporary_file.h"
#include <stdexcept>
#include <unistd.h>

namespace emul::formal {
TemporaryFile::TemporaryFile() {
    char pattern[] = "/tmp/eirc-formal-XXXXXX.btor2";
    auto descriptor = mkstemps(pattern, 6);
    if (descriptor < 0)
        throw std::runtime_error("cannot create formal temporary file");
    close(descriptor);
    path_ = pattern;
}

TemporaryFile::~TemporaryFile() {
    std::error_code error;
    std::filesystem::remove(path_, error);
}
}
