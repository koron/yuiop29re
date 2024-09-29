#pragma once

//////////////////////////////////////////////////////////////////////////////
// Configurations

#ifndef WS2812_ARRAY_NUM
    #error "WS2812_ARRAY_NUM should be defined"
    #define WS2812_ARRAY_NUM 0
#endif
#ifndef WS2812_ARRAY_PIN
    #error "WS2812_ARRAY_PIN should be defined"
    #define WS2812_ARRAY_PIN 0
#endif
#ifndef WS2812_ARRAY_PIO
    #warning "WS2812_ARRAY_PIO is unavailable, pio0 is auto selected"
    #define WS2812_ARRAY_PIO pio0
#endif

//////////////////////////////////////////////////////////////////////////////
// Types

#include <pico/types.h>

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t r;
    uint8_t g;
} ws2812_color_t;

typedef union {
    uint32_t u32;
    ws2812_color_t rgb;
} ws2812_state_t;

//////////////////////////////////////////////////////////////////////////////
// Variables

extern bool ws2812_array_dirty;

extern ws2812_state_t ws2812_array_states[WS2812_ARRAY_NUM];

//////////////////////////////////////////////////////////////////////////////
// Functions

#ifdef __cplusplus
extern "C" {
#endif

// ws2812_array_init() initializes WS2812 LED array.
// It requires a GPIO, a SM of a PIO, and a DMA channel to work.
void ws2812_array_init(void);

// ws2812_array_task transfers bit data to WS2812 LEDs.
// It will do nothign when previos transfer doesn't end.
bool ws2812_array_task(uint64_t now);

// ws2812_array_num is number of LED in the array.
static inline int ws2812_array_num() {
    return WS2812_ARRAY_NUM;
}

// ws2812_array_set_rgb set color of a LED with RGB.
static inline void ws2812_array_set_rgb(int i, uint8_t r, uint8_t g, uint8_t b) {
    ws2812_color_t c =  { .r = r, .g = g, .b = b, .a = 0 };
    ws2812_array_states[i].rgb = c;
    ws2812_array_dirty = true;
}


//----------------------------------------------------------------------------
// Hooks

// ws2812_array_resetdelay_completed is called back when ws2812_array_task can
// be called.
void ws2812_array_resetdelay_completed(void);

#ifdef __cplusplus
}
#endif
