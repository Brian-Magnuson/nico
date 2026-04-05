#include "interop/lib.h"

#include <stdio.h>

void print_secret_message() {
    int secret_number = get_secret_number();
    printf("The secret number is: %d\n", secret_number);
}
