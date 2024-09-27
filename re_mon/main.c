#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

typedef struct {
    uint8_t pinA;
    uint8_t pinB;
    uint8_t state;
    uint8_t lastPins;
    uint64_t changedAt;
} rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t* re, uint a, uint b) {
    re->pinA = a;
    re->pinB = b;
    re->state = 0;
    re->lastPins = 0;
    re->changedAt = 0;

    // Setup two GPIO registers for an encoder.
    gpio_init(a);
    gpio_set_dir(a, GPIO_IN);
    gpio_pull_up(a);
    gpio_init(b);
    gpio_set_dir(b, GPIO_IN);
    gpio_pull_up(b);
}

static struct re_state {
    uint8_t next;
    int8_t out;
} re_states[7*4] = {
    // state: 0 (A, B)
    { 0, 0 },
    { 1, 0 },
    { 4, 0 },
    { 0, 0 },
    // state: 1 (~A, B)
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 2, 0 },
    // state: 2 (~A, ~B)
    { 0, 0 },
    { 0, 0 },
    { 3, 0 },
    { 0, 0 },
    // state: 3 (A, ~B)
    { 0, 1 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    // state: 4 (A, ~B)
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 5, 0 },
    // state: 5 (~A, ~B)
    { 0, 0 },
    { 6, 0 },
    { 0, 0 },
    { 0, 0 },
    // state: 6 (~A, B)
    { 0, -1 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
};

int8_t rotary_encoder_task(rotary_encoder_t *re, uint64_t now) {
    uint32_t curr = gpio_get_all();
    uint8_t pins = ((curr & (1 << re->pinA)) != 0 ? 0 : 1) | ((curr & (1 << re->pinB)) != 0 ? 0 : 2);
    if (pins == re->lastPins || now - re->changedAt < 100) {
        return 0;
    }
    struct re_state st = re_states[re->state * 4 + pins];
    re->state = st.next;
    re->lastPins = pins;
    re->changedAt = now;
    //printf("  state=%d pins=%d out=%d at %llu\n", re->state, pins, st.out, re->changedAt);
    return st.out;
}

static struct re_state2 {
    uint8_t curr;
    uint8_t pins;
    uint8_t next;
    int8_t out;
} re_states2[] = {
    { 0, 1, 1,  0 },
    { 0, 2, 4,  0 },
    { 1, 3, 2,  0 },
    { 2, 2, 3,  0 },
    { 3, 0, 0,  1 },
    { 4, 3, 5,  0 },
    { 5, 1, 6,  0 },
    { 6, 0, 0, -1 },
    { 0, 0, 0,  0 },
};

int8_t rotary_encoder_task2(rotary_encoder_t *re, uint64_t now) {
    uint32_t curr = gpio_get_all();
    uint8_t pins = ((curr & (1 << re->pinA)) != 0 ? 0 : 1) | ((curr & (1 << re->pinB)) != 0 ? 0 : 2);
    if (pins == re->lastPins || now - re->changedAt < 100) {
        return 0;
    }
    struct re_state2 st;
    for (int i = 0; i < sizeof(re_states2) / sizeof(re_states2[0]); i++) {
        st = re_states2[i];
        if (st.curr == re->state && st.pins == pins) {
            break;
        }
    }
    re->state = st.next;
    re->lastPins = pins;
    re->changedAt = now;
    //printf("  state=%d pins=%d out=%d at %llu\n", re->state, pins, st.out, re->changedAt);
    return st.out;
}

int8_t rotary_encoder_task3(rotary_encoder_t *re, uint64_t now) {
    uint32_t curr = gpio_get_all();
    uint8_t pins = ((curr & (1 << re->pinA)) != 0 ? 0 : 1) | ((curr & (1 << re->pinB)) != 0 ? 0 : 2);
    if (pins == re->lastPins || now - re->changedAt < 100) {
        return 0;
    }
    uint8_t next = 0;
    int8_t out = 0;
    switch (re->state) {
        case 0:
            switch (pins) {
                case 1:
                    next = 1;
                    break;
                case 2:
                    next = 4;
                    break;
            }
            break;

        case 1:
            if (pins == 3) {
                next = 2;
            }
            break;
        case 2:
            if (pins == 2) {
                next = 3;
            }
            break;
        case 3:
            if (pins == 0) {
                out = 1;
            }
            break;

        case 4:
            if (pins == 3) {
                next = 5;
            }
            break;
        case 5:
            if (pins == 1) {
                next = 6;
            }
            break;
        case 6:
            if (pins == 0) {
                out = -1;
            }
            break;
    }
    re->state = next;
    re->lastPins = pins;
    re->changedAt = now;
    //printf("  state=%d pins=%d out=%d at %llu\n", re->state, pins, st.out, re->changedAt);
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
        int delta = rotary_encoder_task3(&re1, now);
        if (delta != 0) {
            re_sum = (re_sum + delta + 24) % 24;
            printf("RE: delta=%-2d sum=%-2d at %llu\n", delta, re_sum, now);
        }
        tight_loop_contents();
    }
}
