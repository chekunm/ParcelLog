IMAGE ?= stm32c0-wokwi-build:local

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
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifirmware/Core/Inc \
		tests/test_parcel_rules.c firmware/Core/Src/parcel_rules.c \
		-o tests/build/test_parcel_rules
	tests/build/test_parcel_rules
