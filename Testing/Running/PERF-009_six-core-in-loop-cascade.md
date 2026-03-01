# Test Template

## Test ID
PERF-009

## Test Name
Six-Core In-Loop Cascade — Simultaneous Multi-Core Degradation

## Objective
Extend PERF-003's one-shot multi-core workload into a sustained, per-iteration
burden on all 6 cores simultaneously. PERF-003 added heavy compute ONCE before
`while(1)`. This test adds heavy compute EVERY ITERATION on all 6 cores, with
a shared volatile synchronization counter that forces bus contention between
them.

We want to answer:
- How does per-iteration multi-core loading degrade response time compared
  to the one-shot pattern of PERF-003?
- Does the shared volatile counter cause measurable inter-core interference
  (cache coherency traffic on the crossbar)?
- Is the per-core response time degradation uniform, or does one core become
  the bottleneck due to arbitration?
- Can the analysis model handle 19 new symbols (the largest single-PR symbol
  count in the suite) without errors or timeouts?

## Target PR / Code Delta
- **Branch:** `test/six-core-in-loop-cascade_2026-02-27_10-45`
- **Change:** Add `perf_cascade.c` with 19 new functions. All cores are
  modified to run heavy workloads every iteration.
  - **Per-core workers (12 functions, 2 per core):**
    - `cascade_compute_core0` — 16x16 matrix multiply for core 0
    - `cascade_memory_core0` — 256-element volatile array scan for core 0
    - `cascade_compute_core1` through `cascade_memory_core5` — same pattern
      for cores 1–5 (12 functions total across 6 cores, 2 each)
  - **Shared synchronization (1 function):**
    - `cascade_sync_barrier` — reads and increments a shared volatile
      `uint32_t cascade_shared_counter`. No actual blocking — just a bus
      transaction that all 6 cores execute, creating crossbar contention.
  - **Per-core orchestrators (6 functions):**
    - `cascade_run_core0` through `cascade_run_core5` — each calls its
      core's compute + memory + the shared sync function.
  - **Totals:** 12 workers + 1 sync + 6 orchestrators = 19 new global
    functions.
- **All 6 Cpu files modified:** Each `CpuN_Main.c` adds
  `cascade_run_coreN()` call inside `while(1)` (not one-shot).
- **Scope:** 1 new `.c` + 6 modified caller files. This is the broadest
  file-touch test in the suite after PERF-003.

## Metrics Under Measurement
- [x] Response Time (per core, each measured separately — looking for
  uniform degradation vs single-core bottleneck)
- [x] Throughput (model: single PR, but large delta — 6 files modified)
- [x] Power Usage (on-target: all 6 cores active, crossbar under load,
  highest power test in the suite)
- [x] Degradation Over Time (model: N runs)
- [x] Binary Size (19 new functions; both `Blinky_LED.elf` and
  `multicore.elf` are affected since all Cpu files change)
- [x] Runtime Performance (primary: per-core response time, both binaries)

## Preconditions
- Baseline on `main` HEAD — all cores idle in `while(1)` with only
  `blinkLED()` or equivalent.
- Clean build per Directive #3.
- Same hardware per Directive #5.
- Baseline response times recorded for BOTH `Blinky_LED.elf` AND
  `multicore.elf` (this is the first test in the suite to have meaningful
  multicore.elf data since PERF-003 was one-shot).

## Test Procedure
1. Record baseline on `main` HEAD (both binaries).
2. Create branch `test/six-core-in-loop-cascade_2026-02-27_10-45`.
3. Add `perf_cascade.c` with 19 functions.
4. Modify all 6 `CpuN_Main.c` files to call `cascade_run_coreN()` inside
   each core's `while(1)` loop.
5. Build and verify both binaries compile and link.
6. Commit, push, move test file to `Running/`, open PR.
7. GitHub Actions: clean build, record section sizes for both ELFs.
8. GitHub Actions: flash, on-target timing on all 6 cores, power draw.
9. Record model response time and throughput (6-file delta is larger than
   prior tests — interesting for model processing comparison).
10. Repeat for N runs.
11. Compare all metrics against baseline.

## Input / Workload Description
- **Delta file count:** 7 (1 new + 6 modified) — largest per-PR file count
  in the suite tied with PERF-003.
- **Symbol count:** 19 new global functions — highest in the suite.
- **Per-iteration workload per core:** 16x16 matrix multiply (4096 MACs)
  + 256-element volatile scan + 1 shared counter R/W.
- **Cross-core interaction:** All 6 cores hit `cascade_sync_barrier` every
  iteration. The shared counter creates a serialization point on the
  TC397 crossbar — only one core can complete the write at a time.
- **Nature:** Additive — new file, new per-iteration calls in all cores.

## Expected Results
- **Response time (per core):** >100% increase over baseline for each core.
  The 16x16 matrix multiply (4096 MACs) alone exceeds the ~6 µs baseline
  iteration time at 300 MHz. Adding volatile scan and sync contention
  pushes further degradation.
- **Response time variance across cores:** If the crossbar arbitration
  is fair, all cores should show similar degradation. If one core wins
  arbitration consistently, others will show higher degradation. Document
  the variance — it characterizes TC397 crossbar behavior.
- **Binary .text size:** Significant increase for `multicore.elf` (all
  cores modified). `Blinky_LED.elf` may also grow if it links all Cpu files.
  Estimated +4–6 KB total across both binaries.
- **Power usage:** Highest in the suite. All 6 cores computing simultaneously
  + crossbar traffic. Expected 50–100% increase over baseline power.
- **Model response time:** Higher than single-file tests due to 6-file diff.
  Expected similar to PERF-003 (which also touched 6 Cpu files + 1 source).

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 19 | 12 per-core workers + 1 shared sync + 6 orchestrators |
| New local/static symbols | 0 | No static functions |
| Inlined-away functions | 0 | All noinline |
| Modified existing functions | 6 | `core0_main` through `core5_main` |
| Total function count delta | +19 | Baseline ~171 → expected ~190 |

**New function symbols (all `__attribute__((noinline))`, all GLOBAL):**
Per-core compute workers:
- `cascade_compute_core0` — 16x16 matrix multiply for core 0
- `cascade_compute_core1` through `cascade_compute_core5` (6 total)

Per-core memory workers:
- `cascade_memory_core0` — 256-element volatile scan for core 0
- `cascade_memory_core1` through `cascade_memory_core5` (6 total)

Shared barrier:
- `cascade_sync_barrier` — shared volatile counter increment (1 function)

Per-core orchestrators:
- `cascade_run_core0` through `cascade_run_core5` (6 functions)

**Shared data:** `cascade_shared_counter` — a `volatile uint32_t` in `.data`.
Not a function; does not affect function symbol counts.

**Multi-core note:** Every core runs its orchestrator IN the `while(1)` loop
(not one-shot). This is the key distinction from PERF-003.

## Source-to-Binary Function Correlation

```
Source                           Compilation              ELF Binary
─────                            ───────────              ──────────

perf_cascade.c                    GCC TriCore -O2
┌──────────────────────────┐     ┌──────────────┐    ┌──────────────────────────────┐
│ cascade_compute_core0..5 │────▶│  noinline    │───▶│ cascade_compute_core0..5 [G] │
│ cascade_memory_core0..5  │────▶│  all extern  │───▶│ cascade_memory_core0..5  [G] │
│ cascade_sync_barrier()   │────▶│  discrete    │───▶│ cascade_sync_barrier     [G] │
│ cascade_run_core0..5()   │────▶│  symbols.    │───▶│ cascade_run_core0..5     [G] │
└──────────────────────────┘     └──────────────┘    │                              │
                                                      │ cascade_shared_counter→.data │
Cpu0_Main.c .. Cpu5_Main.c                           │                              │
┌──────────────────────────┐                         │ core0_main..core5_main  [G] │
│ coreN_main()             │                         │   CALL cascade_run_coreN    │
│   while(1) {             │──── CALL ──────────────▶│   CALL blinkLED / yield     │
│     cascade_run_coreN()◄─┼─── EVERY ITERATION      └──────────────────────────────┘
│     blinkLED() / yield() │
│   }                      │      19 new GLOBAL symbols
└──────────────────────────┘       0 inlined / invisible
                                   6 modified (core0_main..core5_main)
                                   1 shared volatile in .data
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Primary:** Response time for each core must be >100% above baseline.
- **Build:** Both `Blinky_LED.elf` AND `multicore.elf` must compile and
  link cleanly.
- **Correctness:** `cascade_shared_counter` must be monotonically
  increasing across all cores. If it decrements or resets, there's a
  race condition — hard fail.
- **Power:** Any increase >150% over baseline triggers a thermal review
  before further testing.
- **Model:** Must process 7-file delta (1 new + 6 modified) without
  timeout or crash.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test is the multi-core counterpart to PERF-006. The comparison is
explicit: PERF-006 adds heavy compute to one core, PERF-009 adds lighter
compute to all 6 cores simultaneously. If per-core response time is similar
between the two, the crossbar contention is negligible. If PERF-009 shows
higher per-core degradation than PERF-006 despite lighter per-core loads,
the crossbar is the bottleneck.

The 19-symbol delta is also the largest in the test suite, making this a
stress case for the analysis tooling. If the symbol count parsing breaks,
it will break here first.

PERF-009 vs PERF-003: PERF-003 was one-shot (heavy compute runs once, then
the loop is fast). PERF-009 is every-iteration (the loop is always heavy).
The on-target response time difference between these two test patterns
directly measures the per-iteration cost vs the one-time initialization cost.
