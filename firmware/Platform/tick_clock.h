#ifndef PARCEL_TICK_CLOCK_H
#define PARCEL_TICK_CLOCK_H
#include <stdint.h>

/* Unsigned modular conversion preserves elapsed intervals below 2^31 us.
 * This does not add resolution to the 1 ms HAL tick. */
static inline uint32_t tick_clock_us(uint32_t milliseconds) {
    return milliseconds * UINT32_C(1000);
}
#endif
