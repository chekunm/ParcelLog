IMAGE ?= parcellog-build:lab2

.PHONY: image build clean test

image:
	docker build -t $(IMAGE) docker

build: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean

test:
	mkdir -p tests/build
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifirmware/App \
		tests/test_parcel_rules.c firmware/App/parcel_rules.c \
		-o tests/build/test_parcel_rules
	tests/build/test_parcel_rules
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifirmware/Devices tests/test_mpu.c firmware/Devices/mpu6050.c -o tests/build/test_mpu
	tests/build/test_mpu

	$(CC) -std=c11 -Wall -Wextra -Werror -DRUNTIME_TEST_PRINTF -Itests/fakes -Ifirmware/Platform -Ifirmware/App -Ifirmware/Devices tests/test_runtime.c firmware/Platform/runtime.c firmware/App/parcel_rules.c firmware/Devices/mpu6050.c -o tests/build/test_runtime
	tests/build/test_runtime
