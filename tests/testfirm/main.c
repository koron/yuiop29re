#include <stdio.h>
#include <string.h>

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
}

static uint8_t oled_buf[SSD1306_BUF_LEN] = {0};

struct render_area oled_frame = {
    .start_col = 0,
    .end_col = SSD1306_WIDTH - 1,
    .start_page = 0,
    .end_page = SSD1306_NUM_PAGES - 1
};

static void oled_render() {
    render(oled_buf, &oled_frame);
};

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

    i2c_init(i2c_default, SSD1306_I2C_CLK * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    SSD1306_init();

    while(true) {
        uint64_t now = time_us_64();

        int delta = rotary_encoder_task(&re1, now);
        if (delta != 0) {
            re_sum = update_re_count(re_sum, delta, ROTALY_ENCODER_1_COUNT);
            printf("re1_changed: delta=%-2d sum=%-2d when=%llu\n", delta, re_sum, now);
        }

        switch_matrix_task(&sm1, now);

        ws2812_array_task(now);

        oled_task(now);

        tight_loop_contents();
    }
}
