#include "runtime.h"
#include "board.h"
#include "parcel_rules.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#ifdef RUNTIME_TEST_PRINTF
int runtime_test_printf(const char *, ...);
#define printf runtime_test_printf
#endif

static Mpu6050 imu;
static ImuSample physical;
static ParcelRules rules;
static ParcelSample sample;
static bool imu_ready, light_ready, led_on, dropping, previous_cr;
static uint32_t sampled_at, status_at, led_at, imu_retry_at, adc_retry_at, sequence;
static char line[64];
static size_t line_size;

static void status(uint32_t now) {
    printf("tick [%lu ms] mode=%s imu=%s raw=%d,%d,%d mg=%ld,%ld,%ld gyro=%ld,%ld,%ld temp_mc=%ld light=%s%u pir=%u tx_overflow=%u\r\n",
           (unsigned long)now, rules.mode == PARCEL_STORAGE ? "storage" : "transit",
           sample.imu_valid ? "ok" : "error", sample.imu_valid ? sample.ax : 0,
           sample.imu_valid ? sample.ay : 0, sample.imu_valid ? sample.az : 0,
           (long)(sample.imu_valid ? physical.accel_mg[0] : 0),
           (long)(sample.imu_valid ? physical.accel_mg[1] : 0),
           (long)(sample.imu_valid ? physical.accel_mg[2] : 0),
           (long)(sample.imu_valid ? physical.gyro_mdps[0] : 0),
           (long)(sample.imu_valid ? physical.gyro_mdps[1] : 0),
           (long)(sample.imu_valid ? physical.gyro_mdps[2] : 0),
           (long)(sample.imu_valid ? physical.temperature_mc : 0),
           sample.light_valid ? "" : "error:", sample.light_valid ? sample.light : 0,
           sample.pir_motion, board_tx_overflow());
}
static void event(uint32_t now, const char *name) {
    printf("seq=%lu time_ms=%lu event=%s mode=%s\r\n", (unsigned long)++sequence,
           (unsigned long)now, name, rules.mode == PARCEL_STORAGE ? "storage" : "transit");
}
static void identify(uint32_t now) {
    BusResult result = mpu6050_init(&imu);
    imu_ready = result == BUS_OK;
    sample.imu_valid = false;
    rules.tilt_samples = 0;
    imu_retry_at = now;
    printf("[%lu ms] selftest addr=0x%02x result=%u %s\r\n", (unsigned long)now,
           imu.address, result, imu_ready ? "mpu6050_ready" : "mpu6050_unavailable");
}
static void command(uint32_t now) {
    line[line_size] = 0;
    if (!strcmp(line, "status")) status(now);
    else if (!strcmp(line, "selftest")) identify(now);
    else if (!strcmp(line, "imuaddr 0x68") || !strcmp(line, "imuaddr 0x69")) {
        imu.address = line[11] == '8' ? 0x68 : 0x69;
        identify(now);
    } else if (line_size) printf("command_error\r\n");
}
static void commands(uint32_t now) {
    if (board_rx_overflow()) { dropping = true; line_size = 0; printf("rx_overflow\r\n"); }
    for (unsigned budget = 0; budget < 128; ++budget) {
        int next = board_get_char();
        if (next < 0) break;
        char ch = (char)next;
        if (ch == '\n' && previous_cr) { previous_cr = false; continue; }
        previous_cr = ch == '\r';
        if (ch == '\n' || ch == '\r') {
            if (!dropping) command(now);
            else printf("command_error\r\n");
            line_size = 0; dropping = false;
        } else if (!dropping) {
            if (!ch || line_size == sizeof(line) - 1) { dropping = true; line_size = 0; }
            else line[line_size++] = ch;
        }
    }
}
void runtime_init(void) {
    board_init();
    imu = (Mpu6050){board_sensor_bus(), 0x68};
    ParcelRules_Init(&rules, PARCEL_TRANSIT);
    sample = (ParcelSample){0}; physical = (ImuSample){0};
    uint32_t now = HAL_GetTick();
    sampled_at = now - 100u; status_at = led_at = now;
    imu_retry_at = now - 2000u; adc_retry_at = now - 1000u;
    imu_ready = light_ready = led_on = dropping = previous_cr = false;
    sequence = 0; line_size = 0;
    printf("ParcelLog ready; GPIO PB0 selects transit/storage; commands: status selftest imuaddr 0x68/0x69\r\n");
}
void runtime_poll(void) {
    uint32_t now = HAL_GetTick();
    commands(now);
    if ((uint32_t)(now - sampled_at) >= 100u) {
        sampled_at = now;
        sample.pir_motion = board_motion();
        ParcelMode mode = board_storage() ? PARCEL_STORAGE : PARCEL_TRANSIT;
        if (mode != rules.mode) {
            ParcelRules_SetMode(&rules, mode, sample.pir_motion);
            event(now, "mode_changed");
        }
        if (!imu_ready && (uint32_t)(now - imu_retry_at) >= 2000u) identify(now);
        sample.imu_valid = false;
        if (imu_ready) {
            BusResult result = mpu6050_read(&imu, &physical);
            if (result == BUS_OK) {
                sample.imu_valid = true;
                sample.ax = physical.accel_raw[0]; sample.ay = physical.accel_raw[1]; sample.az = physical.accel_raw[2];
            } else {
                imu_ready = false; imu_retry_at = now;
                printf("[%lu ms] mpu6050_read_error=%u\r\n", (unsigned long)now, result);
            }
        }
        sample.light_valid = false;
        if (light_ready || (uint32_t)(now - adc_retry_at) >= 1000u) {
            bool was_ready = light_ready;
            light_ready = board_read_light(&sample.light);
            sample.light_valid = light_ready;
            if (!light_ready) adc_retry_at = now;
            if (was_ready != light_ready) event(now, light_ready ? "light_ready" : "light_read_error");
        }
        uint32_t events = ParcelRules_Update(&rules, &sample);
        if (events & PARCEL_EVENT_SHOCK) event(now, "shock");
        if (events & PARCEL_EVENT_TILT) event(now, "tilt");
        if (events & PARCEL_EVENT_LIGHT_CHANGE) event(now, "light_change");
        if (events & PARCEL_EVENT_NEARBY_MOTION) event(now, "nearby_motion");
    }
    if ((uint32_t)(now - status_at) >= 1000u) { status_at = now; status(now); }
    if ((uint32_t)(now - led_at) >= 500u) { led_at = now; led_on = !led_on; board_led(led_on); }
}
