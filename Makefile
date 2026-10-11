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
	$(MAKE) -C tests test
sanitize:
	$(MAKE) -C tests sanitize
test-linux: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) make test sanitize
coverage: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) python3 tools/coverage.py
