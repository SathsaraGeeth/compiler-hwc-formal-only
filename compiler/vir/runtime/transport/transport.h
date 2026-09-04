/*
 * compiler/vir/runtime/transport/transport.h
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
 * 1. This layer abstract the communication between
 *    the VIR runtime and EIR runtime
 * 2. Provides a locked channel between them
 * 3. Methods:
 *      1. Schedule a job
 *      2. Export a signal
 *      3. Import a signal (take an arg, peek=false/ture)
 * 4. Internally it may use seprate queues and other
 *    structures for efficiency
 * 5. This is the only transport-related API
 *    exposed to the compiler.
 * 6. Concrete implementations live in emulation/transport/.
 */

#pragma once

#include <cstdint>
#include <memory>

namespace transport {

using Signal = uint64_t;

struct Value {
    uint64_t data;
    uint64_t xmask;
    uint64_t zmask;
    uint32_t width;
    friend bool operator==(const Value&, const Value&) = default;
};

enum class Status {
    SUCCESS,
    RETRY,
    ERROR
};

struct Job {
    uint64_t    id;
    const char* instance; // the name of the instance e.g. @adder_1001
};

class Transport {
public:
    virtual ~Transport() = default;

    virtual Status try_schedule_job(
        Job& job) = 0;
    virtual Status try_export_signal(
        Signal signal, const Value& value) = 0;
    virtual Status try_import_signal (
        Signal signal, Value& value, bool peek) = 0;
};

} // transport
