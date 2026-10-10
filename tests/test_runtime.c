#include "runtime.h"
#include "board.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static bool storage, pir, adc_ok = true;
static unsigned adc_calls, led_calls;
static uint16_t light = 1000;
static int16_t x;
static char output[20000], input[200];
static size_t output_len, input_pos;
uint32_t HAL_GetTick(void) { return now; }
int runtime_test_printf(const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    int n = vsnprintf(output + output_len, sizeof(output) - output_len, fmt, args);
    va_end(args); assert(n >= 0 && (size_t)n < sizeof(output) - output_len);
    output_len += (size_t)n; return n;
}
void board_init(void) {}
bool board_storage(void) { return storage; }
bool board_motion(void) { return pir; }
bool board_read_light(uint16_t *v) { ++adc_calls; *v = light; return adc_ok; }
void board_led(bool on) { (void)on; ++led_calls; }
int board_get_char(void) { return input[input_pos] ? (unsigned char)input[input_pos++] : -1; }
bool board_rx_overflow(void) { return false; }
bool board_tx_overflow(void) { return false; }
static BusResult read_bus(void *c, uint8_t a, uint8_t reg, uint8_t *data, size_t n) {
    (void)c; if (a == 0x69) return BUS_NACK;
    if (reg == 0x75) { assert(n == 1); data[0] = 0x68; }
    else { assert(reg == 0x3b && n == 14); memset(data, 0, n); data[0] = (uint16_t)x >> 8; data[1] = (uint8_t)x; data[4] = 0x40; }
    return BUS_OK;
}
static BusResult write_bus(void *c, uint8_t a, uint8_t reg, const uint8_t *d, size_t n) {
    (void)c; (void)a; (void)reg; (void)d; (void)n; return BUS_OK;
}
SensorBus board_sensor_bus(void) { return (SensorBus){NULL, read_bus, write_bus}; }
static void send(const char *s) { strcpy(input, s); input_pos = 0; runtime_poll(); input[0] = 0; input_pos = 0; }
int main(void) {
    runtime_init(); runtime_poll();
    assert(strstr(output, "mpu6050_ready"));
    pir = true; storage = true; now = 100; runtime_poll();
    assert(!strstr(output, "nearby_motion"));
    pir = false; now = 200; runtime_poll();
    pir = true; x = 32767; light = 1700; now = 300; runtime_poll();
    assert(strstr(output, "event=shock") && strstr(output, "event=light_change") && strstr(output, "event=nearby_motion"));
    now = 400; runtime_poll(); assert(!strstr(output, "event=tilt"));
    now = 500; runtime_poll(); assert(strstr(output, "event=tilt"));
    send("imuaddr 0x69\r\n"); assert(strstr(output, "addr=0x69 result=1"));
    unsigned before = adc_calls; now = 1000; runtime_poll();
    assert(adc_calls > before && strstr(output, "imu=error") && led_calls);
    send("imuaddr 0x68\n"); assert(strstr(output, "addr=0x68 result=0"));
    send("status\n");
    send("imuaddr 0x68junk\n"); assert(strstr(output, "command_error"));
    send("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nselftest\n");
    adc_ok = false; now = 1100; runtime_poll(); before = adc_calls;
    now = 1200; runtime_poll(); assert(adc_calls == before);
    adc_ok = true; now = 2100; runtime_poll(); assert(adc_calls == before + 1);
    now = UINT32_MAX - 50; runtime_init(); runtime_poll();
    now = 60; runtime_poll();
    puts("runtime: simultaneous events, GPIO mode entry, NACK recovery, ADC retry, parser and tick wrap passed");
    return 0;
}
