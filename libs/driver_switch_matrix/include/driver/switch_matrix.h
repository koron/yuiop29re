#pragma once

#include <pico/types.h>

typedef struct switch_matrix_s switch_matrix_t;

typedef void (*switch_matrix_changed_cb)(switch_matrix_t *sm, uint64_t when, uint state_index, bool on);
typedef void (*switch_matrix_suppressed_cb)(switch_matrix_t *sm, uint8_t when, uint state_index, bool on, uint64_t last_changed);

typedef struct {
    uint8_t  p0;
    uint8_t  p1;
    bool     on:1;
    uint64_t last:63;
} switch_matrix_state_t;

struct switch_matrix_s {
    int num;
    switch_matrix_state_t *states;

    void *user;
    switch_matrix_changed_cb changed;
    switch_matrix_suppressed_cb suppressed;

    uint64_t scan_interval;
    uint32_t select_delay;
    uint32_t unselect_delay;
    uint64_t debounce_interval;

    uint64_t last;
};

void switch_matrix_init(switch_matrix_t *sm);

void switch_matrix_task(switch_matrix_t *sm, uint64_t now);

void switch_matrix_changed(switch_matrix_t *sm, uint64_t when, uint state_index, bool on);

void switch_matrix_suppressed(switch_matrix_t *sm, uint64_t when, uint state_index, bool on, uint64_t last_changed);
