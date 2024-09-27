#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// This driver encodes the states of the A and B pins of the rotary encoder
// into a 2-bit word and checks the past 4 words of history and the current 1
// word to determine if the rotation was successful and the direction of the
// rotation.
//
// The EC12 compatible rotary encoder operates in the sequence of A connection,
// B connection, A disconnection, B disconnection when rotating clockwise. In
// the counterclockwise direction, the sequence is reversed, becoming B
// connection, A connection, B disconnection, A disconnection. This driver
// assumes that C is connected to GND and A and B are in a pulled-up state.
// Therefore, both A and B are in a Hi state initially, and it is expected that
// they will become Lo states upon connection and Hi states upon disconnection.
//
// This driver encodes the Hi state as 0 and the Lo state as 1 when encoding
// the A and B quantity pins to a word. It also assigns A to the 0th bit and B
// to the 1st bit. The resulting 2-bit word and the corresponding relationship
// with the A and B pins are as shown in the following table.
//
// | Word | A | B |
// |-----:|---|---|
// | 00   |Hi |Hi |
// | 10   |Lo |Hi |
// | 11   |Lo |Lo |
// | 01   |Hi |Lo |
//
// Clockwise rotation of the knob starts with 00 as the initial state and the
// 2-bit word transitions in the order of 01, 11, 10, 00. Counterclockwise
// rotation of the knob starts with 00 as the initial state and the 2-bit word
// transitions in the order of 10, 11, 01, 00.
//
// The transition history of this 2-bit word is represented by an 8-bit =
// 1-byte variable using bit shift. Clockwise rotation is 0x1e (=00, 01, 11,
// 10), and counterclockwise rotation is 0x2d (=00, 10, 11, 01). When the
// latest 2-bit word is 00, the direction of rotation is determined by
// comparing the value of one variable indicated by the transition history with
// the aforementioned value, and whether the rotation was successful or not.

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
                // Detected a clockwise rotation.
                out = 1;
                break;
            case 0x2d: /* 0b00_10_11_01 + 0b00 */
                // Detected a counter clockwise rotation.
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
