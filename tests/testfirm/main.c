#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "driver/rotary_encoder.h"
#include "driver/switch_matrix.h"
#include "driver/ws2812_array.h"

#include "hardware/i2c.h"
#include "ssd1306.h"

enum {
    ROW1 = 14,
    ROW2 = 13,
    ROW3 = 12,
    ROW4 = 11,
    ROW5 = 10,

    COL1 =  4,
    COL2 =  5,
    COL3 =  6,
    COL4 =  7,
    COL5 =  8,
    COL6 =  9,
};

static switch_matrix_state_t sm_states[] = {
    { ROW1, COL1 },
    { ROW1, COL2 },
    { ROW1, COL3 },
    { ROW1, COL4 },
    { ROW1, COL5 },
    { ROW1, COL6 },

    { ROW2, COL1 },
    { ROW2, COL2 },
    { ROW2, COL3 },
    { ROW2, COL4 },
    { ROW2, COL5 },
    { ROW2, COL6 },

    { ROW3, COL1 },
    { ROW3, COL2 },
    { ROW3, COL3 },
    { ROW3, COL4 },
    { ROW3, COL5 },
    { ROW3, COL6 },

    { ROW4, COL1 },
    { ROW4, COL2 },
    { ROW4, COL3 },
    { ROW4, COL4 },
    { ROW4, COL5 },
    { ROW4, COL6 },

    { ROW5, COL2 },
    { ROW5, COL3 },
    { ROW5, COL4 },
    { ROW5, COL5 },
    { ROW5, COL6 },

    { ROW5, COL1 },
};

static int update_re_count(int sum, int delta, int max_count) {
    return (sum + delta + max_count) % max_count;
}

static uint8_t oled_buf[SSD1306_BUF_LEN] = {0};

struct render_area oled_frame = {
    .start_col = 0,
    .end_col = SSD1306_WIDTH - 1,
    .start_page = 0,
    .end_page = SSD1306_NUM_PAGES - 1
};

static void oled_init() {
    i2c_init(i2c_default, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    SSD1306_init();

    calc_render_area_buflen(&oled_frame);
    memset(oled_buf, 0, SSD1306_BUF_LEN);
    render(oled_buf, &oled_frame);
}

static void oled_task(uint64_t now) {
    static int mode = 0;
    static uint64_t wait = 0;
    if (wait > 0 && wait > now) {
        return;
    }
    wait = 0;
    switch (mode) {
        case 0:
            SSD1306_send_cmd(SSD1306_SET_ALL_ON);
            mode = 1;
            wait = now + 500 * 1000;
            break;
        case 1:
            static int count_1 = 0;
            SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON);
            if (++count_1 >= 3) {
                mode = 2;
                break;
            }
            mode = 0;
            wait = now + 500 * 1000;
            break;
    }
}

typedef struct {
    float x;
    float y;
} led_pos_t;

static led_pos_t led_positions[] = {
    { 0.0, 0.8 }, { 0.2, 0.8 }, { 0.4, 0.8 }, { 0.6, 0.8 }, { 0.8, 0.8 },
    { 1.0, 0.8 }, { 1.0, 0.6 }, { 0.8, 0.6 }, { 0.6, 0.6 }, { 0.4, 0.6 },
    { 0.2, 0.6 }, { 0.0, 0.6 }, { 0.0, 0.4 }, { 0.2, 0.4 }, { 0.4, 0.4 },
    { 0.6, 0.4 }, { 0.8, 0.4 }, { 1.0, 0.4 }, { 1.0, 0.2 }, { 0.8, 0.2 },
    { 0.6, 0.2 }, { 0.4, 0.2 }, { 0.2, 0.2 }, { 0.0, 0.2 }, { 0.1, 0.0 },
    { 0.3, 0.0 }, { 0.5, 0.0 }, { 0.7, 0.0 }, { 0.9, 0.0 },
};

const uint64_t frac_base_max = (1 << 22) - 1;

typedef void (*led_matrix_get_color_cb)(void *data, ws2812_color_t *c, led_pos_t *pos, uint64_t now);

typedef struct {
    led_matrix_get_color_cb     fn;
    void                        *data;
} led_matrix_get_color_t;

led_matrix_get_color_t led_matrix_get_color = {
    .fn   = NULL,
    .data = NULL
};

void led_matrix_get_color_call(led_matrix_get_color_t *getter, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    if (getter != NULL && getter->fn != NULL) {
        getter->fn(getter->data, c, pos, now);
    }
}

void led_matrix_task(uint64_t now) {
    static uint64_t last = 0;
    if (now - last < 10000) {
        return;
    }
    last = now;
    ws2812_array_dirty = true;
    memset(ws2812_array_states, 0, sizeof(ws2812_array_states));
    for (int i = 0; i < count_of(led_positions); i++) {
        led_matrix_get_color_call(&led_matrix_get_color, &ws2812_array_states[i].rgb, &led_positions[i], now);
    }
}

static void add_color(ws2812_color_t *c, uint8_t r, uint8_t g, uint8_t b) {
    c->r = MAX(c->r, r);
    c->g = MAX(c->g, g);
    c->b = MAX(c->b, b);
}

static void get_vertical_rainbow_color(void *data, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    const uint8_t L = 255;
    float frac = (float)(now & frac_base_max) / (float)frac_base_max;
    float hue = fmod(frac + pos->x / 8.0, 1.0) * 6.0;
    uint8_t v = (uint8_t)(fmod(hue, 1.0) * L);
    uint8_t L_v = L - v;
    switch (((int)hue) % 6) {
        case 0: add_color(c, L,   v,   0  ); break;
        case 1: add_color(c, L_v, L,   0  ); break;
        case 2: add_color(c, 0,   L,   v  ); break;
        case 3: add_color(c, 0,   L_v, L  ); break;
        case 4: add_color(c, v,   0,   L  ); break;
        case 5: add_color(c, L,   0,   L_v); break;
    }
}

#define GETTERS_MAX 16

typedef struct {
    led_matrix_get_color_t colors[GETTERS_MAX];
} getters_t;

static getters_t color_getters = {0};

void color_getters_get_color(void *data, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    getters_t *p = (getters_t*)data;
    for (int i = 0; i < GETTERS_MAX; i++) {
        led_matrix_get_color_call(&p->colors[i], c, pos, now);
    }
}

int color_provider_has(led_matrix_get_color_cb fn, void *data) {
    for (int i = 0; i < GETTERS_MAX; i++) {
        if (color_getters.colors[i].fn == fn && color_getters.colors[i].data == data) {
            return i;
        }
    }
    return -1;
}

int color_provider_add(led_matrix_get_color_cb fn, void *data) {
    for (int i = 0; i < GETTERS_MAX; i++) {
        if (color_getters.colors[i].fn == NULL) {
            color_getters.colors[i].fn   = fn;
            color_getters.colors[i].data = data;
            return i;
        }
    }
    return -1;
}

void color_provider_remove(int i) {
    if (i >= 0 && i < GETTERS_MAX) {
        color_getters.colors[i].fn   = NULL;
        color_getters.colors[i].data = NULL;
    }
}

static int sm_to_led_index[] = {
     0,  1,  2,  3,  4,  5,
    11, 10,  9,  8,  7,  6,
    12, 13, 14, 15, 16, 17,
    23, 22, 21, 20, 19, 18,
      24, 25, 26, 27, 28,
};

static void on_sm_changed(switch_matrix_t *sm, uint64_t when, uint state_index, bool on) {
    printf("sm%d_changed: state_index=%-2d %-3s when=%llu\n", (int)sm->user, state_index, on ? "ON" : "OFF", when);
    if (state_index < count_of(sm_to_led_index)) {
        int led_index = sm_to_led_index[state_index];
        if (on) {
            ws2812_array_set_rgb(led_index, 255, 255, 255);
        } else {
            ws2812_array_set_rgb(led_index, 0, 0, 0);
        }
    }
    if (state_index == 29 && on) {
        int i = color_provider_has(get_vertical_rainbow_color, NULL);
        if (i < 0) {
            color_provider_add(get_vertical_rainbow_color, NULL);
        } else {
            color_provider_remove(i);
        }
    }
}

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: testfirm\n");

    rotary_encoder_t re1;
    rotary_encoder_init(&re1, ROTALY_ENCODER_1_PIN_A, ROTALY_ENCODER_1_PIN_B);
    int re_sum = 0;

    switch_matrix_t sm1 = {
        .num     = count_of(sm_states),
        .states  = &sm_states[0],
        .user    = (void *)1,
        .changed = on_sm_changed,
    };
    switch_matrix_init(&sm1);

    ws2812_array_init();

    led_matrix_get_color.data = &color_getters;
    led_matrix_get_color.fn = color_getters_get_color;
    //get_vertical_rainbow_color;

    oled_init();

    while(true) {
        uint64_t now = time_us_64();

        int delta = rotary_encoder_task(&re1, now);
        if (delta != 0) {
            re_sum = update_re_count(re_sum, delta, ROTALY_ENCODER_1_COUNT);
            printf("re1_changed: delta=%-2d sum=%-2d when=%llu\n", delta, re_sum, now);
        }

        switch_matrix_task(&sm1, now);

        led_matrix_task(now);

        ws2812_array_task(now);

        oled_task(now);

        tight_loop_contents();
    }
}
