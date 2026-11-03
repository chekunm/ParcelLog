#include <assert.h>
#include <string.h>
#include "sda_fault.chip.c"
static timer_config_t timer_config;
static uint32_t wait_us;
static int sda_mode = -1, marker_value = -1;
pin_t pin_init(const char *name, uint32_t mode) {
    if (!strcmp(name, "SDA")) { sda_mode = mode; return 1; }
    assert(!strcmp(name, "FAULT")); marker_value = LOW; return 2;
}
void pin_mode(pin_t pin, uint32_t mode) { assert(pin == 1); sda_mode = mode; }
void pin_write(pin_t pin, uint32_t value) { assert(pin == 2); marker_value = value; }
timer_t timer_init(const timer_config_t *config) { timer_config = *config; return 1; }
void timer_start(timer_t timer, uint32_t micros, bool repeat) { assert(timer == 1 && !repeat); wait_us = micros; }
int main(void) {
    chip_init();
    assert(sda_mode == INPUT && marker_value == LOW && wait_us == 8000000);
    timer_config.callback(timer_config.user_data);
    assert(sda_mode == OUTPUT_LOW && marker_value == HIGH && wait_us == 3000000);
    timer_config.callback(timer_config.user_data);
    assert(sda_mode == INPUT && marker_value == LOW);
    return 0;
}
