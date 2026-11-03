# Register-bank fixture

This test double implements only the register transactions needed by this firmware. It returns a stationary +1 g Z sample, stores configuration writes, and allows address NACKs and bad WHO_AM_I reads at startup through `initial_nacks` and `bad_identity_reads`. It does not model sensor physics, time-dependent failures, SDA held low, or power loss.

The adjacent diagram uses two initial NACKs and one bad identity result. `wokwi.toml` targets the separately built `firmware/build-independent` image. A corrected build was exercised in Wokwi; see the owner audit `Lab03/Work/independent-revision-20261101/STATUS.md` for the matching hashes and observed behavior. It is not S1, S2, S3, or hardware evidence.

Build with the existing local `wokwi/builder-clang-wasm:latest` image:

```sh
docker run --rm -v "$PWD:/work" -w /work wokwi/builder-clang-wasm:latest clang --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--allow-undefined -Wl,--export=chip_init -Wl,--export-table -o tests/wokwi-imu-fixture/register_bank.chip.wasm tests/wokwi-imu-fixture/register_bank.chip.c
```

## Separate SDA fault scenario

`diagram-sda-fault.json` adds an independent injector on PB9/SDA and places SCL, SDA, UART_TX and FAULT on logic channels 0–3. `wokwi-sda-fault.toml` declares both chips and points at the frozen isolated ELF. Prepare an isolated Wokwi project containing these two files as its `diagram.json` and `wokwi.toml`, plus the paths referenced by the TOML; do not replace the owner's root diagram or old captures. The injector starts released, drives SDA LOW at simulation time 8 s, asserts FAULT, releases SDA at 11 s and lowers FAULT. It is an artificial digital fault, not a physical MPU model. The marked interval and wire level must both be observed; a marker alone is not proof that the simulator resolved the bus as intended.

The test-bank configuration also injects two startup address NACKs and one wrong identity read. Expected checks, subject to actual simulator observations: initial `mpu6050_unavailable`, then a later `mpu6050_ready`; during asserted SDA, an explicit timeout/bus error and `imu_fault` if the sensor had reached ready; after release, subsequent retry and `mpu6050_ready` with valid samples. Capture raw UART and VCD continuously, hash the exact ELF/configuration and report observed timestamps; a timeout, continued bus busy, or failed recovery is a failed check, not a result to infer away. This firmware resets the I2C peripheral on failure; it does not implement nine SCL recovery pulses, so it cannot guarantee recovery from every physical stuck slave.

The chip callbacks and phase timing were unit-tested with host fakes (also under ASan/UBSan). The later corrected build was exercised in Wokwi; the marker, SDA level and recovery were observed in the recorded artificial digital-fault scenario. This does not establish physical behavior. Both `.wasm` files compile with `wokwi/builder-clang-wasm:latest`; build the injector by replacing the input and output names in the command above with `sda_fault.chip.c` and `sda_fault.chip.wasm`.

The first Wokwi attempt used modules built without `--export-table`; callbacks could not be established as running and the fault marker stayed low. Rebuilt modules export the indirect function table required by the documented Wokwi build command. Their simulator behavior must be judged from the separate rerun, not inferred from the export alone.
