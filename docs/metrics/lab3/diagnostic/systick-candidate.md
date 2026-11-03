# SysTick candidate, 29 October 2026

The TIM3 snapshot evidence does not establish why an interval resembled 2^32 minus 65384 us. Its read masks interrupts while collecting `timer_high`, `CNT` and `UIF`, with a pending-overflow adjustment; that review does not prove the sampled clock is monotonic on the target. The candidate changes the time source, not the unexplained TIM3 mechanism. It does not clamp an observed backward value.

With `PARCEL_CLOCK_SYSTICK`, `instrument_now` reads HAL's 32-bit millisecond tick and returns its unsigned product by 1000. The TIM3 clock and interrupt are not enabled in this mode. This has 1 ms resolution; 0 us task readings are quantized and are not a WCET measurement. Time under WFI is not a reliable CPU load measurement in this candidate: WFI starts disabled. If interrupts are masked or SysTick cannot execute long enough for multiple ticks to coalesce, HAL time can lag wall/VCD time. HAL tick can also stop during stalled execution or debugging. These limitations require target checks, especially during actual flash operations.

The modular 32-bit microsecond representation wraps about every 71.6 minutes, while HAL's 32-bit millisecond counter wraps about every 49.7 days. Elapsed differences are unambiguous only when observations are frequent enough to distinguish wrap from a long interruption; signed scheduling comparisons require intervals below 2^31 us. Host `test_tick_clock` reproduces the observed near-2^32 unsigned result from a 65384 us backward step and verifies normal wraps at both boundaries without a clamp. Host tests do not establish that the target tick runs correctly.

Candidate source SHA256 `3baa36f7992671ba510ef9661f8a751b80d83f3a77b1868de7bff8144d263432` and per-file hashes are in `source-manifest.json`. The flag `-DPARCEL_CLOCK_SYSTICK` is an additional build input, so the ELF hash is the definitive variant identity. The candidate was built with Ubuntu 24.04 image `parcellog-build:lab3`, ARM GCC 13.2, `make -C firmware all BUILD_DIR=build-systick-candidate 'C_DEFS=-DUSE_HAL_DRIVER -DSTM32C031xx -DPARCEL_CLOCK_SYSTICK'`.

- Candidate ELF `firmware/build-systick-candidate/firmware.elf`: SHA256 `2b22b7cc8a8a7a47687667399a58bd45a28b6be3d93b43c37b479bc78aabe6b2`.
- Candidate HEX `firmware/build-systick-candidate/firmware.hex`: SHA256 `3f46d586a9dd0b08d30b4eb74cc10fc065df1ba17e330e30bc6eefd53b230858`.
- Linker: 28388 bytes in 28 KiB code flash (284 bytes free), 8056 bytes linked RAM including reserves.
- Seven host suites passed in default and candidate compilation modes; default host ASan/UBSan also passed. The suite's simulated S2 is not a one-hour MCU run.

## Target validation conditions

1. The candidate uses its exact ELF/HEX and a unique VCD path in a short run without debugger interventions. Confirm the wire banner `timer=SysTick_1ms`, source prefix, candidate ELF hash and actual MCU identity. Correlate clock increments, UART events and VCD edges over multiple periods and across an actual long operation. Investigate missing SysTick increments or IRQ blocking rather than compensating the clock.
2. Verify the journal's HAL result, target readback, CRC and reset as separate facts. Preserve the observed all-zero behavior if it remains. No fabricated erase delay or substitute RAM storage.
3. If the short gate is valid, perform the complete applicable S1/S2/S3 and fault suite on one frozen candidate. Require the actual one-hour S2, thousands of relevant samples, at least 30 real events during a genuine long operation, and runtime memory. Use independent VCD for short latencies; do not claim coarse 0 us values or busy-loop CPU estimates as successful measurements. A demonstrated failure remains a finding.
4. Archive raw UART/VCD, session timing, configuration and ELF provenance before each subsequent launch. A failed or incomplete gate is not acceptance.
