#include "tick_clock.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    /* The observed suspicious TIM3 subtraction is a backwards step of 65384. */
    uint32_t earlier = 1000000u, backwards = earlier - 65384u;
    assert((uint32_t)(backwards - earlier) == UINT32_C(4294901912));

    /* With a coherent advancing 1 ms tick, a similar elapsed interval is 65000 us. */
    assert((uint32_t)(tick_clock_us(1065) - tick_clock_us(1000)) == 65000u);
    /* The microsecond product wraps after about 71 minutes, including across wrap. */
    assert((uint32_t)(tick_clock_us(4294968u) - tick_clock_us(4294966u)) == 2000u);
    /* HAL's own 32-bit millisecond counter wraps after about 49.7 days. */
    assert((uint32_t)(tick_clock_us(1u) - tick_clock_us(UINT32_MAX - 1u)) == 3000u);
    /* Equal ticks are legitimate at 1 ms resolution; they are not zero WCET. */
    assert(tick_clock_us(42u) == tick_clock_us(42u));
    puts("clock: backward unsigned failure reproduced; SysTick modular wraps checked on host");
}
