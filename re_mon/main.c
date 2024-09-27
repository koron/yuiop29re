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
// Therefore, both A and B are expected to be in a Hi state initially, and to
// become Lo states upon connection and Hi states upon disconnection.
//
// This driver encodes the Hi state as 0 and the Lo state as 1 when encoding
// both pins A and B to a word. It also assigns A to the 0th bit and B to the
// 1st bit. The resulting 2-bit word and the corresponding relationship between
// pins A and B are as shown in the following table.
//
// | Word | B | A |
// |-----:|---|---|
// | 00   |Hi |Hi |
// | 01   |Hi |Lo |
// | 11   |Lo |Lo |
// | 10   |Lo |Hi |
//
// Clockwise rotation of the knob starts with 00 as the initial state and the
// 2-bit word transitions in the order of 01, 11, 10, 00. Counterclockwise
// rotation of the knob starts with 00 as the initial state and the 2-bit word
// transitions in the order of 10, 11, 01, 00.
//
// The past four words of the two-bit word transition history are packed into
// an 8-bit = 1-byte variable in order from the least significant bit using bit
// shift. Clockwise rotation becomes 0x1e (=00, 01, 11, 10), and
// counterclockwise rotation becomes 0x2d (=00, 10, 11, 01). When the latest
// two-bit word is 00, the rotation is judged to be successful or not and the
// direction of the rotation is determined by comparing the variable indicating
// the transition history with the above values.

typedef struct {
    uint8_t pinA;
    uint8_t pinB;
    uint8_t history;
    uint64_t changedAt;
} rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t* re, uint a, uint b) {
    // Setup two GPIO registers for an encoder.
    uint mask = (1 << a) | (1 << b);
    gpio_init_mask(mask);
    gpio_set_dir_in_masked(mask);
    gpio_pull_up(a);
    gpio_pull_up(b);

    // Initialize rotary_encoder_t's fields.
    re->pinA = a;
    re->pinB = b;
    re->history = 0;
    re->changedAt = 0;
}

int8_t rotary_encoder_task(rotary_encoder_t *re, uint64_t now) {
    // All GPIO bits are inverted beforehand, and then the desired A and B bits
    // are extracted and combined as a 2-bit word.
    uint32_t curr = ~gpio_get_all();
    uint8_t word = ((curr >> re->pinA) & 1) |
                   ((curr >> re->pinB) & 1) << 1;
    // If the current 2-bit word is different from the previous 2-bit word, and
    // more than 250μs have passed since the last update for debouncing, the
    // state is updated.
    if (word == (re->history & 0x03) || now - re->changedAt < 250) {
        return 0;
    }
    //printf("  history=%02X word=%X\n", re->history, word);
    int8_t out = 0;
    if (word == 0) {
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
    re->history = re->history << 2 | word;
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
