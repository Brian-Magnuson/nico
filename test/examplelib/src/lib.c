#include "examplelib/lib.h"

#include <stdarg.h>

int examplelib_get_constant() {
    return 42;
}

int examplelib_square(int x) {
    return x * x;
}

int examplelib_add(int a, int b) {
    return a + b;
}

int examplelib_sum(int count, ...) {
    va_list args;
    va_start(args, count);
    int total = 0;
    for (int i = 0; i < count; ++i) {
        total += va_arg(args, int);
    }
    va_end(args);
    return total;
}
