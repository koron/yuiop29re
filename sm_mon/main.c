#include <stdio.h>

#include "pico/stdlib.h"
#include "driver/switch_matrix.h"

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

int main() {
    stdio_init_all();
    printf("\nYUIOP29RE: Switch Matrix monitor\n");

    switch_matrix_t sm1 = {
        .num    = count_of(sm_states),
        .states = &sm_states[0],
        .user   = (void *)1,
    };
    switch_matrix_init(&sm1);

    while(true) {
        uint64_t now = time_us_64();
        switch_matrix_task(&sm1, now);
        tight_loop_contents();
    }
}
