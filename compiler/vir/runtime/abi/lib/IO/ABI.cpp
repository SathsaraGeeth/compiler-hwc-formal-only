#include "IO/DescriptorIO.h"
#include <cstdint>

namespace {
vir::runtime::io::DescriptorIO descriptor_io;
}

extern "C" bool vir_io_read_i1(std::int32_t, const char*)
    asm("vir.runtime.io.read.i1");
extern "C" bool vir_io_read_i1(std::int32_t descriptor, const char* name) {
    return descriptor_io.read(descriptor, name, 1) != 0;
}

extern "C" std::uint8_t vir_io_read_i8(std::int32_t, const char*)
    asm("vir.runtime.io.read.i8");
extern "C" std::uint8_t vir_io_read_i8(std::int32_t descriptor, const char* name) {
    return static_cast<std::uint8_t>(descriptor_io.read(descriptor, name, 8));
}

extern "C" void vir_io_write_i1(std::int32_t, const char*, bool, bool, bool)
    asm("vir.runtime.io.write.i1");
extern "C" void vir_io_write_i1(std::int32_t descriptor, const char* name,
                                bool value, bool xmask, bool zmask) {
    descriptor_io.write(descriptor, name, value, xmask, zmask, 1);
}

extern "C" void vir_io_write_i8(std::int32_t, const char*, std::uint8_t,
                                 std::uint8_t, std::uint8_t)
    asm("vir.runtime.io.write.i8");
extern "C" void vir_io_write_i8(std::int32_t descriptor, const char* name,
                                std::uint8_t value, std::uint8_t xmask,
                                std::uint8_t zmask) {
    descriptor_io.write(descriptor, name, value, xmask, zmask, 8);
}
