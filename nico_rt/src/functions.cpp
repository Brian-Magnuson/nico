#include "nico_rt/functions.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

extern "C" int nico_rt_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    std::vector<char> buffer(256);

    va_list args_copy;
    va_copy(args_copy, args);

    int size = std::vsnprintf(buffer.data(), buffer.size(), format, args_copy);

    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        return -1;
    }

    if (static_cast<size_t>(size) >= buffer.size()) {
        buffer.resize(static_cast<size_t>(size) + 1);

        std::vsnprintf(buffer.data(), buffer.size(), format, args);
    }

    va_end(args);

    std::cout.write(buffer.data(), size);
    std::cout.flush();

    return size;
}

extern "C" int nico_rt_printerrf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    std::vector<char> buffer(256);

    va_list args_copy;
    va_copy(args_copy, args);

    int size = std::vsnprintf(buffer.data(), buffer.size(), format, args_copy);

    va_end(args_copy);

    if (size < 0) {
        va_end(args);
        return -1;
    }

    if (static_cast<size_t>(size) >= buffer.size()) {
        buffer.resize(static_cast<size_t>(size) + 1);

        std::vsnprintf(buffer.data(), buffer.size(), format, args);
    }

    va_end(args);

    std::cerr.write(buffer.data(), size);
    std::cerr.flush();

    return size;
}

extern "C" void nico_rt_abort() {
    std::abort();
}

extern "C" void nico_rt_exit(int exit_code) {
    std::exit(exit_code);
}

extern "C" void* nico_rt_malloc(size_t size) {
    return std::malloc(size);
}

extern "C" void nico_rt_free(void* ptr) {
    std::free(ptr);
}

extern "C" void nico_rt_setjmp() {
    // setjmp is actually a standard function-like macro.
    setjmp(nico_rt_jmp_buf);
}

extern "C" void nico_rt_longjmp() {
    std::longjmp(nico_rt_jmp_buf, 1);
}
