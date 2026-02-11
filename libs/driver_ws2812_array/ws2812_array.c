#include <string.h>

#include "driver/ws2812_array.h"

#include "pico/sem.h"
#include "hardware/dma.h"

#include "ws2812.pio.h"

bool ws2812_array_dirty = false;

ws2812_state_t ws2812_array_states[WS2812_ARRAY_NUM] = {0};
static ws2812_state_t sendbuf[WS2812_ARRAY_NUM] = {0};

// reset delay for NeoPixel should be longer than 80us
static const uint resetdelay_us = 100;

static uint         dma_chan;
static io_rw_32     dma_chan_mask;
static alarm_id_t   resetdelay_alarm = 0;
static struct       semaphore resetdelay_sem;

__attribute__((weak)) void ws2812_array_resetdelay_completed(void) {}

static int64_t on_completed_resetdelay(alarm_id_t id, void *user_data) {
    resetdelay_alarm = 0;
    sem_release(&resetdelay_sem);
    // notify reset delay completed to user function.
    ws2812_array_resetdelay_completed();
    return 0; // no repeat
}

static void __isr on_completed_dma() {
    if (dma_hw->ints0 & dma_chan_mask) {
        // clear IRQ0 status register bit
        dma_hw->ints0 = dma_chan_mask;
        if (resetdelay_alarm != 0) {
            cancel_alarm(resetdelay_alarm);
        }
        resetdelay_alarm = add_alarm_in_us(resetdelay_us,
                on_completed_resetdelay, NULL, true);
    }
}

void ws2812_array_init(void) {
    sem_init(&resetdelay_sem, 1, 1);

    PIO pio = WS2812_ARRAY_PIO;
    int sm = pio_claim_unused_sm(pio, true);

    // setup PIO/SM with ws2812 program.
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_ARRAY_PIN, 800000);

    // setup DMA to send LED data.
    int chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(chan, &c, &pio->txf[sm], NULL,
            count_of(sendbuf), false);

    // enalbe IRQ0 at DMA trasfer completed.
    irq_set_exclusive_handler(DMA_IRQ_0, on_completed_dma);
    dma_channel_set_irq0_enabled(chan, true);
    irq_set_enabled(DMA_IRQ_0, true);

    dma_chan = chan;
    dma_chan_mask = 1u << chan;
}

const uint64_t MAX_TOTAL_LEVEL = WS2812_ARRAY_MAX_CURRENT * 255 / WS2812_ARRAY_CURRENT_PER_CHANNEL;

static inline uint8_t muldiv8_32(uint8_t v, uint32_t mul, uint32_t div) {
    return (uint8_t)((uint32_t)v * mul / div);
}

static void apply_autocap(ws2812_state_t *p, int n) {
    if (MAX_TOTAL_LEVEL == 0) {
        return;
    }
    uint32_t total = 0;
    for (int i = 0; i < n; i++) {
        ws2812_color_t c = p[i].rgb;
        total += c.b;
        total += c.r;
        total += c.g;
    }
    if (total <= MAX_TOTAL_LEVEL) {
        return;
    }
    for (int i = 0; i < n; i++) {
        ws2812_color_t *c = &p[i].rgb;
        c->r = muldiv8_32(c->r, MAX_TOTAL_LEVEL, total);
        c->g = muldiv8_32(c->g, MAX_TOTAL_LEVEL, total);
        c->b = muldiv8_32(c->b, MAX_TOTAL_LEVEL, total);
    }

}

bool ws2812_array_task(uint64_t now) {
    if (!ws2812_array_dirty) {
        return false;
    }
    if (!sem_acquire_timeout_ms(&resetdelay_sem, 0)) {
        return false;
    }
    ws2812_array_dirty = false;
    // Copy the DMA target to the send buffer to protect it from being
    // overwritten.
    memcpy(sendbuf, ws2812_array_states, sizeof(sendbuf));
    apply_autocap(sendbuf, count_of(sendbuf));
    dma_channel_set_read_addr(dma_chan, sendbuf, true);
    return true;
}
