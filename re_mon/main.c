#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

typedef struct {
    uint8_t pinA;
    uint8_t pinB;
    uint8_t history;
    uint64_t changedAt;
} rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t* re, uint a, uint b) {
    re->pinA = a;
    re->pinB = b;
    re->history = 0;
    re->changedAt = 0;

    // Setup two GPIO registers for an encoder.
    gpio_init(a);
    gpio_set_dir(a, GPIO_IN);
    gpio_pull_up(a);
    gpio_init(b);
    gpio_set_dir(b, GPIO_IN);
    gpio_pull_up(b);
}

int8_t rotary_encoder_task(rotary_encoder_t *re, uint64_t now) {
    uint32_t curr = gpio_get_all();
    uint8_t pins =
        ((curr & (1 << re->pinA)) != 0 ? 0 : 1) |
        ((curr & (1 << re->pinB)) != 0 ? 0 : 2);
    if (pins == (re->history & 0x03) || now - re->changedAt < 250) {
        return 0;
    }
    //printf("  history=%02X pins=%X\n", re->history, pins);
    int8_t out = 0;
    if (pins == 0) {
        switch (re->history) {
            case 0x1e: /* 0b00_01_11_10 + 0b00 */
                out = 1;
                break;
            case 0x2d: /* 0b00_10_11_01 + 0b00 */
                out = -1;
                break;
        }
    }
    re->history = re->history << 2 | pins;
    re->changedAt = now;
    return out;
}

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
