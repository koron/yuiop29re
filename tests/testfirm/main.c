#define PERFCOUNT_LED_MATRIX_TASK       0
#define FEATURE_LED_WHILE_PRESSING      0
#define FEATURE_RAINBOW                 1

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

static int re_sum = 0;

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
    static int last_re_sum = -1;
    static char re_msg[30] = {0};

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
        case 2:
            if (last_re_sum == re_sum) {
                break;
            }
            last_re_sum = re_sum;
            snprintf(re_msg, sizeof(re_msg), "RE sum %-2d", re_sum);
            WriteString(oled_buf, 0, 0, re_msg);
            float sec = (float)now / 1000000;
            snprintf(re_msg, sizeof(re_msg), "    at %.2f", sec);
            WriteString(oled_buf, 0, 9, re_msg);
            render(oled_buf, &oled_frame);
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

typedef void (*led_matrix_get_color_cb)(void *data, int idx, ws2812_color_t *c, led_pos_t *pos, uint64_t now);

typedef struct {
    led_matrix_get_color_cb     fn;
    void                        *data;
} led_matrix_get_color_t;

led_matrix_get_color_t led_matrix_get_color = {
    .fn   = NULL,
    .data = NULL
};

void led_matrix_get_color_call(led_matrix_get_color_t *getter, int idx, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    if (getter != NULL && getter->fn != NULL) {
        getter->fn(getter->data, idx, c, pos, now);
    }
}

static void add_color(ws2812_color_t *c, uint8_t r, uint8_t g, uint8_t b);

static void get_white_color(void *data, int idx, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    add_color(c, 255, 255, 255);
}

void led_matrix_task(uint64_t now) {
    static uint64_t last = 0;
    if (now - last < 10000) {
        return;
    }

#if PERFCOUNT_LED_MATRIX_TASK
    static uint64_t sum   = 0;
    static uint64_t count = 0;
    uint64_t start = time_us_64();
#endif

    last = now;
    ws2812_array_dirty = true;
    memset(ws2812_array_states, 0, sizeof(ws2812_array_states));
    for (int i = 0; i < count_of(led_positions); i++) {
        led_matrix_get_color_call(&led_matrix_get_color, i, &ws2812_array_states[i].rgb, &led_positions[i], now);
    }

#if PERFCOUNT_LED_MATRIX_TASK
    sum += time_us_64() - start;
    if (count++ >= 100) {
        printf("led_matrix_task: performance %llu\n", sum / count);
        sum = 0;
        count = 0;
    }
#endif
}

static void add_color(ws2812_color_t *c, uint8_t r, uint8_t g, uint8_t b) {
    c->r = MAX(c->r, r);
    c->g = MAX(c->g, g);
    c->b = MAX(c->b, b);
}

static inline float fract(float x) {
    return x - (int)x;
}

static void get_vertical_rainbow_color(void *data, int idx, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    const uint8_t L = 255;
    float frac = (float)(now & frac_base_max) / (float)frac_base_max;
    float hue = fract(frac + pos->x / 8.0) * 6.0;
    uint8_t v = (uint8_t)(fract(hue) * L);
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

#define GETTERS_MAX 30

typedef struct {
    led_matrix_get_color_t colors[GETTERS_MAX];
} getters_t;

static getters_t color_getters = {0};

void color_getters_get_color(void *data, int idx, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    getters_t *p = (getters_t*)data;
    for (int i = 0; i < GETTERS_MAX; i++) {
        led_matrix_get_color_call(&p->colors[i], idx, c, pos, now);
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

static float time_reduction(float x, float t) {
    if (x >= 1.0)
        return 1.0;
    float denom = 1.0 - t;
    if (denom < 1e-7)
        denom = 1e-7;
    float k = 1.0 / denom;
    // A fast approximation of powf(x, k)
    union { float f; int32_t i; } u;
    u.f = x;
    u.i = (int32_t)(k * (u.i - 0x3f7a3bea) + 0x3f7a3bea);
    return u.f;
}

typedef struct {
    int led_index;
    uint64_t start;

    uint64_t cached_start;
    uint64_t cached_now;
    float cached_t;
} push_effect_t;

push_effect_t push_effects[29] = {0};

// A fast approximation of powf(0.2, x)
static float fast_pow_02(float x) {
    if (x < 1e-7) {
        return 1.0f;
    }
    float y = x * -2.321928f;
    union { float f; int i; } u;
    u.i = (int)((1 << 23) * (y + 126.942695f));
    return u.f;
}

static void light_effect_on_switch_press(void *data, int idx, ws2812_color_t *c, led_pos_t *pos, uint64_t now) {
    push_effect_t *p = (push_effect_t *)data;
    led_pos_t *center = &led_positions[p->led_index];
    float dx = pos->x - center->x;
    float dy = pos->y - center->y;
    float r = sqrtf(dx * dx + dy * dy);
    if (p->cached_now != now || p->cached_start != p->start) {
        p->cached_start = p->start;
        p->cached_now = now;
        p->cached_t = MIN((float)(now - p->start) / 1e6, 1);
    }
    float f = time_reduction(fast_pow_02(r / 0.2), p->cached_t);
    uint8_t v = (uint8_t)(255 * f);
    if (v > 0) {
        add_color(c, v, v, v);
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
#if FEATURE_LED_WHILE_PRESSING
        if (on) {
            push_effect_t effect = { .led_index=led_index, .start=when };
            push_effects[led_index] = effect;
            color_provider_add(light_effect_on_switch_press, (void *)&push_effects[led_index]);
        } else {
            int i = color_provider_has(light_effect_on_switch_press, (void *)&push_effects[led_index]);
            if (i >= 0) {
                color_provider_remove(i);
            }
        }
#else
        if (on) {
            int i = color_provider_has(light_effect_on_switch_press, (void *)&push_effects[led_index]);
            if (i >= 0) {
                color_provider_remove(i);
            } else {
                push_effect_t effect = { .led_index=led_index, .start=when };
                push_effects[led_index] = effect;
                color_provider_add(light_effect_on_switch_press, (void *)&push_effects[led_index]);
            }
        }
#endif
    }
    if (state_index == 29 && on) {
#if FEATURE_RAINBOW
        static led_matrix_get_color_cb cb = get_vertical_rainbow_color;
#else
        static led_matrix_get_color_cb cb = get_white_color;
#endif
        int i = color_provider_has(cb, NULL);
        if (i < 0) {
            color_provider_add(cb, NULL);
        } else {
            color_provider_remove(i);
        }
    }
}

static int update_re_count(int sum, int delta, int max_count) {
    return (sum + delta + max_count) % max_count;
}

static void on_re_changed(rotary_encoder_t *re, uint64_t when, int8_t delta) {
    re_sum = update_re_count(re_sum, delta, ROTALY_ENCODER_1_COUNT);
    printf("re1_changed: delta=%-2d sum=%-2d when=%llu\n", delta, re_sum, when);
}

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: testfirm\n");

    rotary_encoder_t re1 = {
        .user    = (void *)0,
        .changed = on_re_changed,
    };
    rotary_encoder_init(&re1, ROTALY_ENCODER_1_PIN_A, ROTALY_ENCODER_1_PIN_B);

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

    oled_init();

    while(true) {
        uint64_t now = time_us_64();

        rotary_encoder_task(&re1, now);

        switch_matrix_task(&sm1, now);

        led_matrix_task(now);

        ws2812_array_task(now);

        oled_task(now);

        tight_loop_contents();
    }
}
