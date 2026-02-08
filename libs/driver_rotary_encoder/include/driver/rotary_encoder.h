#pragma once

#include <pico/types.h>

typedef struct rotary_encoder_s rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t *re, uint a, uint b);

int8_t rotary_encoder_task(rotary_encoder_t *re, uint64_t now);

typedef void (*rotary_encoder_changed_cb)(rotary_encoder_t *re, uint64_t when, int8_t delta);

struct rotary_encoder_s {
    void *user;
    rotary_encoder_changed_cb changed;

    uint8_t pinA;
    uint8_t pinB;

    uint8_t history;
    uint64_t changedAt;
};
