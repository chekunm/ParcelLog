# Diagnostic run, 29 October 2026

This is an intrusive debugger experiment, not an accepted endurance or performance scenario. The simulator was stopped after its debugger had halted the CPU.

## Source and capture

The wire-decoded startup banner in `vcd-analysis.wire-uart.json` says `source=b8047132268a8f5e`; the ELF checked immediately before opening the simulator had SHA256 `977e94918ed5b94f5e2de34d17f9dd8f2c379f26e469a286ec40db19b0e7b155`. The VCD SHA256 is `49b861db6e47645500b76675ef33a7a8deb5f39040d89bfde318e2da1ee76811`. It contains 21.107960308 s of simulated time and 181 complete SAMPLE pulses. UART collection began after simulation startup and captured 7600 bytes in 45.358336459 s of wall time; it does not contain the initial banner. The VCD's wire decoding does contain that banner.

## Debugger phases

The actual packet transcript, raw registers, full 2048-byte reads of each page, and wall timestamps are retained under `gdb/`. The initial halt occurred after the machine had already run, not at reset. During the three-second busy phase sample_runs changed 151 to 181, command_runs 14543 to 17459, and TIM3 IRQ count 229 to 275. After GDB set `instrument_use_wfi=1`, sample_runs remained 181 and command_runs remained 17459, while TIM3 IRQ count increased from 275 to 322. The final snapshot was halted by the probe. These observations associate the WFI phase with missing foreground progression in this particular interrupted run. They do not establish a spontaneous stall, its root cause, or validity of timing around debugger pauses.

## Flash

The last captured diagnostic state reports erase stage 4, HAL erase status 0, HAL error 0 and PageError 0xffffffff. Both full pages read as 2048 zero bytes in each snapshot. The actual UART reports `erase_ok=0`, `intact=0`, `damaged=128`, `next=128`, and `save_failed=4`. In the source, `shipment_erase` calls the HAL-backed erase, reopens/scans the journal, and returns success only when the scan finds `next_slot==0`. The HAL success and product-level failed verification are therefore distinct observations. A completed erase duration marker or HAL status alone does not prove erased bytes or durability. Debugger reads and target reads both need independent confirmation before attributing the result to a specific simulator or hardware behavior.

## Clock

VCD SAMPLE periods have 180 intervals averaging 99943.917 us, with minimum 76497.950 us and maximum 114185.650 us; they are digital edge intervals from this short instrumented run. UART metrics report `elapsed_hi=4`, `elapsed_lo=15100336` and `loop max_us=4294901912`, inconsistent with accepting the roughly 21 s VCD as reliable accumulated elapsed time. The reported loop maximum is 65384 us below 2^32. `runtime_poll` accumulates unsigned `(current - last_loop)` into a 64-bit elapsed total; a backward sample would add nearly 2^32 us, but a backward timer reading has not been observed directly and the cause remains unproven. GDB pause effects and overflow accounting must be isolated before changing the clock or treating a clamp as a root-cause repair.

## Next gate

A separately built candidate requires with explicit monotonicity/overflow diagnostics and a host regression for backward samples and legitimate wrap. Preserve the full frozen diagnostic image and raw evidence. Validate timestamp transitions against VCD without debugger pauses, separately verify program/erase/readback and reset, and only then select a final build for S1/S2/S3. WFI must remain a measured concern, not a claimed fix. No simulated delay should be substituted for a genuine long operation.
