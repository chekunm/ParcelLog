#include "wokwi-api.h"

typedef enum { READY, DELAY, HIGH_PULSE, REFRACTORY } Phase;
typedef struct { pin_t flash, pir; timer_t timer; Phase phase; } State;
static State state;

static void tick(void *opaque) {
    State *s = opaque;
    if (s->phase == DELAY) {
        if (pin_read(s->flash) == HIGH) {
            pin_write(s->pir, HIGH);
            s->phase = HIGH_PULSE;
            timer_start(s->timer, 200000, false);
        } else s->phase = READY;
    } else if (s->phase == HIGH_PULSE) {
        pin_write(s->pir, LOW);
        s->phase = REFRACTORY;
        timer_start(s->timer, 200000, false);
    } else if (s->phase == REFRACTORY) s->phase = READY;
}
static void on_flash(void *opaque, pin_t pin, uint32_t value) {
    State *s = opaque;
    (void)pin;
    if (value == HIGH && s->phase == READY) {
        s->phase = DELAY;
        timer_start(s->timer, 100, false);
    }
}
void chip_init(void) {
    state.flash = pin_init("FLASH", INPUT);
    state.pir = pin_init("PIR", OUTPUT_LOW);
    state.phase = READY;
    const timer_config_t timer = {.user_data=&state, .callback=tick};
    state.timer = timer_init(&timer);
    const pin_watch_config_t watch = {.user_data=&state, .edge=RISING, .pin_change=on_flash};
    pin_watch(state.flash, &watch);
}
