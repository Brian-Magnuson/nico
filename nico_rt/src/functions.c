#include "nico_rt/functions.h"

#include <stdarg.h>
#include <stdio.h>

extern int nico_rt_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}
