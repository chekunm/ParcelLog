IMAGE ?= parcellog-build:lab3

.PHONY: image build clean test

image:
	docker build -t $(IMAGE) docker

build: image
	python3 tools/seal_build.py
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean

test:
	python3 tools/check_project.py
	$(MAKE) -C tests test
	python3 tests/test_rsp_probe.py
sanitize:
	$(MAKE) -C tests sanitize
test-linux: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) make test sanitize
