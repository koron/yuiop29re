#pragma once

#include <pico/types.h>

typedef struct {
    uint8_t pinA;
    uint8_t pinB;
    uint8_t history;
    uint64_t changedAt;
} rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t* re, uint a, uint b);

int8_t rotary_encoder_task(rotary_encoder_t *re, uint64_t now);
