#include <stdio.h>

#include "pico/stdlib.h"
#include "driver/rotaryencoder.h"

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: Rotaly Encoder monitor\n");

    rotary_encoder_t re1;
    rotary_encoder_init(&re1, ROTALY_ENCODER_1_PIN_A, ROTALY_ENCODER_1_PIN_B);

    int re_sum = 0;
    while(true) {
        uint64_t now = time_us_64();
        int delta = rotary_encoder_task(&re1, now);
        if (delta != 0) {
            re_sum = (re_sum + delta + 24) % 24;
            printf("RE: delta=%-2d sum=%-2d at %llu\n", delta, re_sum, now);
        }
        tight_loop_contents();
    }
}
