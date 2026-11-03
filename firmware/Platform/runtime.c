#include "runtime.h"
#include "board.h"
#include "parcel_rules.h"
#include "main.h"
#include "instrument.h"
#include "shipment_log.h"
#include "run_meter.h"
#include "build_version.h"
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
static uint32_t imu_retry_at, adc_retry_at, sequence;
static ShipmentLog journal;
static ShipmentEntry pending[8];
static uint8_t pending_head, pending_tail;
static uint32_t log_dropped, log_failed, rx_lost, epoch, last_loop;
static uint64_t elapsed_us;
static bool exporting, stats_printing;
static uint16_t export_slot, export_limit;
static unsigned stat_index;
static const char *const event_names[] = {"boot", "mode_changed", "shock", "tilt", "light_change", "nearby_motion", "light_ready", "light_read_error", "imu_fault", "inspection", "imu_ready"};
enum { COMMAND, SAMPLE, STATUS, LED, SAVE, EXPORT, LOOP, RESPONSE, PIR_LATENCY, METER_COUNT };
static const char *const task_names[] = {"command", "sample", "status", "led", "save", "export", "loop", "command_latency", "pir_latency"};
static RunMeter meters[METER_COUNT], frozen[METER_COUNT];
static Release releases[4], frozen_releases[4];
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
    uint32_t reacted = instrument_now();
    instrument_probe(8, true);
    if (!strcmp(name, "nearby_motion")) meter_add(&meters[PIR_LATENCY], reacted - board_motion_timestamp());
    unsigned kind = 0;
    while (kind < 10 && strcmp(name, event_names[kind])) ++kind;
    uint8_t next = (uint8_t)((pending_head + 1u) & 7u);
    if (next == pending_tail) ++log_dropped;
    else {
        pending[pending_head] = (ShipmentEntry){.uptime_ms = now, .kind = (uint8_t)kind,
            .mode = (uint8_t)rules.mode, .valid = (sample.imu_valid ? 1u : 0u) | (sample.light_valid ? 2u : 0u),
            .light = sample.light, .ax = sample.ax, .ay = sample.ay, .az = sample.az};
        pending_head = next;
    }
    printf("seq=%lu time_ms=%lu event=%s mode=%s t_us=%lu\r\n", (unsigned long)++sequence,
           (unsigned long)now, name, rules.mode == PARCEL_STORAGE ? "storage" : "transit", (unsigned long)instrument_now());
    instrument_probe(8, false);
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
    else if (!strcmp(line, "mark")) event(now, "inspection");
    else if (!strcmp(line, "stats")) {
        memcpy(frozen, meters, sizeof meters); memcpy(frozen_releases, releases, sizeof releases);
        stats_printing = true; stat_index = 0;
        uint64_t asleep = instrument_sleep_us();
        uint32_t used, free_bytes; instrument_heap_usage(&used, &free_bytes);
#ifdef PARCEL_CLOCK_SYSTICK
        printf("METRICS clock=SysTick_1ms elapsed_hi=%lu elapsed_lo=%lu sleep_hi=%lu sleep_lo=%lu stack_watermark=%lu stack_reserve=2048 heap_arena=%lu heap_used=%lu heap_free=%lu rx_overflows=%lu queue_dropped=%lu save_failed=%lu intact=%u damaged=%u next=%u\r\n",
#else
        printf("METRICS clock=TIM3_1MHz elapsed_hi=%lu elapsed_lo=%lu sleep_hi=%lu sleep_lo=%lu stack_watermark=%lu stack_reserve=2048 heap_arena=%lu heap_used=%lu heap_free=%lu rx_overflows=%lu queue_dropped=%lu save_failed=%lu intact=%u damaged=%u next=%u\r\n",
#endif
            (unsigned long)(elapsed_us >> 32), (unsigned long)elapsed_us,
            (unsigned long)(asleep >> 32), (unsigned long)asleep,
            (unsigned long)instrument_stack(), (unsigned long)instrument_heap(),
            (unsigned long)used, (unsigned long)free_bytes, (unsigned long)rx_lost,
            (unsigned long)log_dropped, (unsigned long)log_failed, journal.intact, journal.damaged, journal.next_slot);
    } else if (!strcmp(line, "log")) {
        if (!exporting) { exporting = true; export_slot = 0; export_limit = journal.next_slot; }
    } else if (!strcmp(line, "erase CONFIRM")) {
        if (exporting || pending_head != pending_tail) printf("erase=busy\r\n");
        else {
            uint32_t t = instrument_now(); instrument_probe(7, true);
            bool ok = shipment_erase(&journal); instrument_probe(7, false);
            meter_add(&meters[SAVE], instrument_now() - t);
            printf("erase_ok=%u t_us=%lu\r\n", ok, (unsigned long)(instrument_now() - t));
        }
    }
    else if (!strcmp(line, "imuaddr 0x68") || !strcmp(line, "imuaddr 0x69")) {
        imu.address = line[11] == '8' ? 0x68 : 0x69;
        identify(now);
    } else if (line_size) printf("command_error\r\n");
}
static void commands(uint32_t now) {
    if (board_rx_overflow()) { ++rx_lost; dropping = true; line_size = 0; printf("rx_overflow\r\n"); }
    if (board_tx_free() < 512u) return;
    for (unsigned budget = 0; budget < 128; ++budget) {
        int next = board_get_char();
        if (next < 0) break;
        char ch = (char)next;
        if (ch == '\n' && previous_cr) { previous_cr = false; continue; }
        previous_cr = ch == '\r';
        if (ch == '\n' || ch == '\r') {
            uint32_t received = board_rx_timestamp();
            if (!dropping) command(now);
            else printf("command_error\r\n");
            meter_add(&meters[RESPONSE], instrument_now() - received);
            line_size = 0; dropping = false;
            break;
        } else if (!dropping) {
            if (!ch || line_size == sizeof(line) - 1) { dropping = true; line_size = 0; }
            else line[line_size++] = ch;
        }
    }
}
void runtime_init(void) {
    board_init();
    shipment_open(&journal, board_shipment_storage());
    imu = (Mpu6050){board_sensor_bus(), 0x68};
    ParcelRules_Init(&rules, PARCEL_TRANSIT);
    sample = (ParcelSample){0}; physical = (ImuSample){0};
    uint32_t now = HAL_GetTick();
    imu_retry_at = now - 2000u; adc_retry_at = now - 1000u;
    imu_ready = light_ready = led_on = dropping = previous_cr = false;
    sequence = 0; line_size = 0;
    pending_head = pending_tail = 0; log_dropped = log_failed = rx_lost = 0;
    exporting = stats_printing = false; elapsed_us = 0;
    memset(meters, 0, sizeof meters);
    epoch = last_loop = instrument_now(); elapsed_us = epoch;
    releases[COMMAND] = (Release){.period=1000, .deadline=2000, .due=epoch};
    releases[SAMPLE] = (Release){.period=100000, .deadline=120000, .due=epoch};
    releases[STATUS] = (Release){.period=1000000, .deadline=1100000, .due=epoch+1000000};
    releases[LED] = (Release){.period=500000, .deadline=550000, .due=epoch+500000};
#ifdef PARCEL_CLOCK_SYSTICK
    printf("BUILD git=%s source=%s clock_hz=%lu timer=SysTick_1ms format=1\r\n", BUILD_GIT, BUILD_SOURCE, (unsigned long)SystemCoreClock);
#else
    printf("BUILD git=%s source=%s clock_hz=%lu timer=TIM3_1MHz format=1\r\n", BUILD_GIT, BUILD_SOURCE, (unsigned long)SystemCoreClock);
#endif
    event(now, "boot");
    printf("ParcelLog ready; PB0=mode; status selftest imuaddr 0x68/0x69 mark log stats erase CONFIRM\r\n");
}
static void sample_task(uint32_t now) {
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
                event(now, "imu_fault");
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
static void save_task(void) {
    if (pending_head == pending_tail) return;
    uint32_t start = instrument_now(); instrument_probe(7, true);
    ShipmentEntry *entry = &pending[pending_tail];
    bool ok = shipment_append(&journal, entry);
    instrument_probe(7, false);
    meter_add(&meters[SAVE], instrument_now() - start);
    if (!ok) ++log_failed;
    /* Failure is explicit, not retried forever. FIFO remains available to sensors. */
    pending_tail = (uint8_t)((pending_tail + 1u) & 7u);
}
static void output_task(void) {
    if (board_tx_free() < 512u) return;
    uint32_t start = instrument_now();
    if (stats_printing) {
        unsigned i = stat_index++;
        RunMeter *m = &frozen[i];
        printf("STAT task=%s n=%lu min_us=%lu avg_us=%lu max_us=%lu", task_names[i],
            (unsigned long)m->n, (unsigned long)m->min,
            (unsigned long)(m->n ? m->sum/m->n : 0), (unsigned long)m->max);
        if (i < 4) {
            Release *r = &frozen_releases[i];
            printf(" jitter_min_us=%ld jitter_max_us=%ld skipped=%lu deadline_miss=%lu",
                (long)r->jitter_min, (long)r->jitter_max, (unsigned long)r->skipped, (unsigned long)r->late);
        }
        printf("\r\n");
        stats_printing = stat_index < METER_COUNT;
    } else if (exporting) {
        ShipmentEntry e;
        if (export_slot == export_limit) { exporting = false; printf("LOG end\r\n"); }
        else {
            unsigned slot = export_slot++;
            if (shipment_read(&journal, (uint16_t)slot, &e))
                printf("LOG slot=%u seq=%lu boot=%lu ms=%lu kind=%s mode=%u valid=%u adc=%u xyz=%d,%d,%d crc=ok\r\n",
                    slot, (unsigned long)e.sequence, (unsigned long)e.boot, (unsigned long)e.uptime_ms,
                    event_names[e.kind], e.mode, e.valid, e.light, e.ax, e.ay, e.az);
            else printf("LOG slot=%u invalid\r\n", slot);
        }
    } else return;
    meter_add(&meters[EXPORT], instrument_now() - start);
}
void runtime_poll(void) {
    uint32_t current = instrument_now();
    elapsed_us += (uint32_t)(current - last_loop);
    meter_add(&meters[LOOP], current - last_loop); last_loop = current;
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t start = instrument_now();
        if (!release_ready(&releases[i], start)) continue;
        uint32_t now = (uint32_t)((elapsed_us + (uint32_t)(instrument_now() - last_loop))/1000u);
        if (i == COMMAND) commands(now);
        else if (i == SAMPLE) { instrument_probe(6, true); sample_task(now); instrument_probe(6, false); }
        else if (i == STATUS) status(now);
        else { led_on = !led_on; board_led(led_on); }
        uint32_t finish = instrument_now();
        meter_add(&meters[i], finish - start); release_finish(&releases[i], finish);
    }
    save_task(); output_task();
}
