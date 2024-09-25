#include <stdio.h>

#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: Hello World\n");
    while(true) {
        tight_loop_contents();
    }
}
