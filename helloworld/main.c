#include <stdio.h>

#include "pico/stdlib.h"

#include "ledarray.h"

static int clip_start = 0;
static int clip_end = LEDARRAY_NUM;

static void update_rainbow(uint t) {
    uint level = 0;
    //uint level = (t / LEDARRAY_NUM) % 7;
    for (int i = clip_start; i < clip_end; i++) {
        uint8_t r = 0, g = 0, b = 0;
        float h = (float)((i + t) % LEDARRAY_NUM)/ (float)LEDARRAY_NUM * 6;
        int phase = (int)h;
        uint8_t f = (uint8_t)(255 * (h - (float)phase));
        switch (phase) {
            default:
            case 0:
                r = 255;
                g = f;
                b = 0;
                break;
            case 1:
                r = 255 - f;
                g = 255;
                b = 0;
                break;
            case 2:
                r = 0;
                g = 255;
                b = f;
                break;
            case 3:
                r = 0;
                g = 255 - f;
                b = 255;
                break;
            case 4:
                r = f;
                g = 0;
                b = 255;
                break;
            case 5:
                r = 255;
                g = 0;
                b = 255 - f;
                break;
        }
        ledarray_set_rgb(i, r >> level, g >> level, b >> level);
    }
}

void light_task(uint64_t now) {
    static uint64_t last = 0;
    static uint32_t state = 0;
    if (now - last < 33 * 1000) {
        return;
    }
    last = now;
    update_rainbow(state);
    state++;
}

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: Hello World\n");
    ledarray_init();
    while(true) {
        uint64_t now = time_us_64();
        light_task(now);
        ledarray_task(now);
        tight_loop_contents();
    }
}
