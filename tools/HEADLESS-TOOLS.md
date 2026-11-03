# Headless measurement tools

`serial_capture.py` uses pySerial 3.5 (BSD-3-Clause, https://github.com/pyserial/pyserial/blob/v3.5/LICENSE.txt), pinned in `requirements-capture.txt`. It writes exact received bytes and a host-monotonic command/session record. Host seconds do not establish MCU time. The session records connection errors and interrupted runs; a process killed without cleanup can still leave it marked recording. `tests/test_serial_capture.py` exercises raw bytes, schedule validation, and connection failure with fakes; it is not an RFC2217 integration test.

For new VCD protocol inspection, use `sigrok-cli` 0.7.2 with libsigrok 0.5.2 and libsigrokdecode 0.5.3, GPL-2.0-or-later for the UART and I2C decoders (https://github.com/sigrokproject/sigrok-cli). The locally installed versions decoded synthetic UART byte 0x41 and I2C address 0x68/ACK. Example for appropriately named signals:

```sh
sigrok-cli -i capture.vcd -I vcd -P uart:rx=UART_TX:baudrate=115200 --protocol-decoder-samplenum -A uart=rx-data
sigrok-cli -i capture.vcd -I vcd -P i2c:scl=SCL:sda=SDA --protocol-decoder-samplenum -A i2c=addr-data
```

VCD import can expand time into sampled data. For large captures establish available memory and use deliberate downsampling only after validating that timing resolution remains suitable; sample numbers must be converted using the actual resulting sample rate. Inspect raw edges as well as decoder output. These commands are protocol inspection, not substitutes for task jitter, latency, CPU, S2 or persistence measurements. Signal names depend on the actual capture.

The legacy `analyze_vcd.py` and `coverage.py` were removed from active tools after substantial cross-project overlap was identified. Hash-verified historical copies are kept outside the repository in the owner's Lab03 audit. For future coverage, use the matching public GNU gcov or gcovr, record instrumented source scope and raw `.gcno`/`.gcda`, and do not claim board/HAL coverage from portable-module tests. No automatic coverage target is currently provided. The previous VCD analyzer could omit a final UART frame; sigrok's protocol output does not itself measure all project-specific timing.

For a new, explicitly scoped host coverage report with public gcovr 8.3 in Ubuntu 24.04, run from the repository root. This instruments separate test executables and aggregates them; it does not reproduce exactly the old custom runner's percentages. It requires network access to install the named packages inside the disposable container and does not include board/HAL code. Save the generated `.gcno`, `.gcda` and JSON with the compiler/gcov version and exact test inputs. It runs the host fake-time scenarios, including a fake S2, which is not a simulator or physical endurance run.

```sh
docker run --rm -v "$PWD:/work" -w /work parcellog-build:lab3 sh -c '
  set -eu
  apt-get update
  apt-get install -y --no-install-recommends python3-venv
  python3 -m venv /tmp/coverage-env
  /tmp/coverage-env/bin/pip install gcovr==8.3
  gcc --version
  gcov --version
  /tmp/coverage-env/bin/gcovr --version
  make -C tests test BUILD=build/gcov CFLAGS="-std=c11 -O0 -g -Wall -Wextra -Werror --coverage"
  /tmp/coverage-env/bin/gcovr --root /work --filter "firmware/(Devices/mpu6050|App/(parcel_rules|shipment_log|run_meter))\\.c$" --print-summary --json-summary /work/tests/build/gcov/coverage-summary.json /work/tests/build/gcov
'
```

No new coverage run was made for this instruction. Historical percentages require the archived script, the original compiler and the original inputs; do not treat the new scope as numerically interchangeable with them.
