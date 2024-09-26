#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: GPIO monitor\n");

    for (int i = 0; i < 30; i++) {
#ifdef PICO_DEFAULT_UART_TX_PIN
        if (i == PICO_DEFAULT_UART_TX_PIN) {
            continue;
        }
#endif
#ifdef PICO_DEFAULT_UART_RX_PIN
        if (i == PICO_DEFAULT_UART_RX_PIN) {
            continue;
        }
#endif
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);
    }

    uint64_t last_change = 0;
    uint32_t last_gpio = 0;
    while(true) {
        uint64_t now = time_us_64();
        uint32_t curr = gpio_get_all() & 0x3ffffffe;
        if (last_gpio != curr && now - last_change >= 5000) {
            printf("GPIO: %08lx at %llu\n", curr, now);
            last_gpio = curr;
            last_change = now;
        }
        tight_loop_contents();
    }
}
