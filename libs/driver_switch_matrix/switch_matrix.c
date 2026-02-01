#include <stdio.h>

#include "driver/switch_matrix.h"
#include "hardware/gpio.h"

#include "pico/stdlib.h"

//////////////////////////////////////////////////////////////////////////////
// Pre-declarations

static void sm_gpio_init(uint gpio);
static void sm_scan_switches(switch_matrix_t *sm, uint64_t now);
static void sm_performance_count(switch_matrix_t *sm, uint64_t now);
static void sm_set_switch_state(switch_matrix_t *sm, uint64_t now, uint knum, bool on);

//////////////////////////////////////////////////////////////////////////////
// Public functions

void switch_matrix_init(switch_matrix_t *sm) {
    uint32_t inited_pins = 0;
    for (uint i = 0; i < sm->num; i++) {
        uint8_t p0 = sm->states[i].p0, p1 = sm->states[i].p1;
        if ((inited_pins & (1 << p0)) == 0) {
            sm_gpio_init(p0);
            inited_pins |= 1 << p0;
        }
        if ((inited_pins & (1 << p1)) == 0) {
            sm_gpio_init(p1);
            inited_pins |= 1 << p1;
        }
    }
    if (sm->scan_interval == 0) {
        sm->scan_interval = 500;
    }
    if (sm->select_delay == 0) {
        sm->select_delay = 1;
    }
    if (sm->unselect_delay == 0) {
        sm->unselect_delay = 1;
    }
    if (sm->debounce_interval == 0) {
        sm->debounce_interval = 10 * 1000;
    }
}

void switch_matrix_task(switch_matrix_t *sm, uint64_t now) {
    if (now - sm->last < sm->scan_interval) {
        return;
    }
    sm->last = now;
    sm_scan_switches(sm, now);
    sm_performance_count(sm, now);
}

__attribute__((weak)) void switch_matrix_changed(switch_matrix_t *sm, uint64_t when, uint state_index, bool on) {
    if (sm->changed != NULL) {
        sm->changed(sm, when, state_index, on);
        return;
    }
    printf("switch_matrix_changed: state_index=%-2d %-3s when=%llu\n", state_index, on ? "ON" : "OFF", when);
}

__attribute__((weak)) void switch_matrix_suppressed(switch_matrix_t *sm, uint64_t when, uint state_index, bool on, uint64_t last_changed) {
    if (sm->suppressed != NULL) {
        sm->suppressed(sm, when, state_index, on, last_changed);
        return;
    }
    printf("switch_matrix_suppressed: state_index=%-2d %-3s when=%llu last=%llu elapsed=%llu\n", state_index, on ? "ON" : "OFF", when, last_changed, when - last_changed);
}

//////////////////////////////////////////////////////////////////////////////
// Private functions

void sm_gpio_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
    gpio_put(gpio, false);
}

void sm_scan_switches(switch_matrix_t *sm, uint64_t now) {
    uint32_t scanned[32] = {0};
    for (uint i = 0; i < sm->num; i++) {
        uint8_t p0 = sm->states[i].p0;
        uint8_t p1 = sm->states[i].p1;
        if (scanned[p0] == 0) {
            gpio_set_dir(p0, GPIO_OUT);
            busy_wait_us_32(sm->select_delay);
            scanned[p0] = gpio_get_all();
            if (scanned[p0] == 0) {
                scanned[p0] = ~0;
            }
            gpio_set_dir(p0, GPIO_IN);
            busy_wait_us_32(sm->unselect_delay);
        }
        bool on = (scanned[p0] & (1 << p1)) == 0;
        sm_set_switch_state(sm, now, i, on);
    }
}

void sm_set_switch_state(switch_matrix_t *sm, uint64_t now, uint state_index, bool on) {
    switch_matrix_state_t *st = &sm->states[state_index];
    if (on == st->on) {
        return;
    }
    uint64_t last = st->last << 1;
    uint64_t elapsed = now - last;
    if (elapsed >= sm->debounce_interval) {
        st->on = on;
        st->last = now >> 1;
        switch_matrix_changed(sm, now, state_index, on);
    } else {
        switch_matrix_suppressed(sm, now, state_index, on, last);
    }
}

void sm_performance_count(switch_matrix_t *sm, uint64_t now) {
    // TODO:
}
