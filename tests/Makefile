CC ?= cc
PLATFORM := $(shell uname -s)-$(shell uname -m)
BUILD ?= build/$(PLATFORM)
CFLAGS ?= -std=c11 -O0 -g -Wall -Wextra -Werror
CPPFLAGS += -I../firmware/Devices -I../firmware/App -I../firmware/Platform -Ifakes
HEADERS = $(wildcard ../firmware/App/*.h ../firmware/Devices/*.h ../firmware/Platform/*.h fakes/*.h fixtures/*.h)
SUITES = test_mpu test_parcel_rules test_runtime test_shipment_log test_run_meter test_runtime_scenarios test_tick_clock
.PHONY: test sanitize
test: $(SUITES:%=$(BUILD)/%)
	@set -e; for suite in $(SUITES); do ./$(BUILD)/$$suite; done
$(BUILD):
	mkdir -p "$@"
$(BUILD)/test_mpu: test_mpu.c ../firmware/Devices/mpu6050.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o "$@"
$(BUILD)/test_parcel_rules: test_parcel_rules.c ../firmware/App/parcel_rules.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o "$@"
$(BUILD)/test_runtime: test_runtime.c ../firmware/Platform/runtime.c ../firmware/App/parcel_rules.c ../firmware/App/shipment_log.c ../firmware/App/run_meter.c ../firmware/Devices/mpu6050.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRUNTIME_TEST_PRINTF $(filter %.c,$^) -o "$@"
sanitize:
	$(MAKE) test BUILD=build/$(PLATFORM)/sanitize CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer'

$(BUILD)/test_shipment_log: test_shipment_log.c ../firmware/App/shipment_log.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o "$@"
$(BUILD)/test_run_meter: test_run_meter.c ../firmware/App/run_meter.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o "$@"
$(BUILD)/test_tick_clock: test_tick_clock.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o "$@"

$(BUILD)/test_runtime_scenarios: test_runtime_scenarios.c ../firmware/Platform/runtime.c ../firmware/App/parcel_rules.c ../firmware/App/shipment_log.c ../firmware/App/run_meter.c ../firmware/Devices/mpu6050.c $(HEADERS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DRUNTIME_TEST_PRINTF $(filter %.c,$^) -o "$@"
