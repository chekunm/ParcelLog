#include "../wokwi-flash-trigger/wokwi-api.h"

/* The fault is injected independently of the register-bank device. */
typedef struct { pin_t sda, marker; timer_t timer; bool asserted; } Fault;
static Fault fault;
static void phase_change(void *context) {
    Fault *f = context;
    if (!f->asserted) {
        pin_mode(f->sda, OUTPUT_LOW);
        pin_write(f->marker, HIGH);
        f->asserted = true;
        timer_start(f->timer, 3000000, false);
    } else {
        pin_mode(f->sda, INPUT);
        pin_write(f->marker, LOW);
    }
}
void chip_init(void) {
    fault.sda = pin_init("SDA", INPUT);
    fault.marker = pin_init("FAULT", OUTPUT_LOW);
    const timer_config_t config = { .user_data = &fault, .callback = phase_change };
    fault.timer = timer_init(&config);
    timer_start(fault.timer, 8000000, false);
}
