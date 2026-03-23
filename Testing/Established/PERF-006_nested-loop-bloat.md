# Test Template

## Test ID
PERF-006

## Test Name
Nested Loop Bloat — Per-Iteration Heavy Compute

## Objective
Push the main loop iteration time past 2x baseline by adding deeply nested
arithmetic workloads that execute every iteration. Unlike PERF-003 (which
ran its workload once before the loop) and PERF-001 (which used 8x8 matrices),
this test uses 32x32 matrix operations and additional stride-pattern scans
that run on every pass through `while(1)`.

We want to answer:
- Can we reliably achieve 100%+ response time degradation with in-loop
  compute rather than one-shot compute?
- How do multiply-accumulate-heavy loops behave on TC397's integer pipeline?
- Does the hardware show a linear relationship between loop body work and
  response time, or does it plateau at a pipeline bottleneck?
- Can the analysis tools correctly attribute degradation to a known set of
  new symbols when there are many of them?

## Target PR / Code Delta
- **Branch:** `test/nested-loop-bloat_2026-02-27_10-00`
- **Change:** Add `perf_bloat.c` containing 7 functions, all `noinline`,
  all called from `core0_main`'s `while(1)` loop via an orchestrator.
  - `bloat_matrix32_multiply` — 32x32 integer matrix multiply (32768 MACs
    per call). No shortcuts, no loop unrolling hints — straight triple-nested
    loop so the compiler emits a predictable but expensive sequence.
  - `bloat_dot_product` — dot product of two 256-element arrays.
  - `bloat_array_accumulate` — sum all elements of a 512-element volatile
    array. Volatile forces a load per element, defeating auto-vectorization.
  - `bloat_stride_scan` — read a 1024-element array with stride 7 (prime
    stride to defeat prefetch). Returns XOR of touched elements.
  - `bloat_outer_product` — compute a 16x16 outer product (256 multiplies),
    store result in a local array, sum it, return the sum.
  - `bloat_nested_sum` — triple-nested loop summing a 16x16x4 3D array
    element by element.
  - `bloat_run_all` — orchestrator. Calls all six above in sequence every
    iteration.
- **Scope:** Single new file + one caller modification.

## Metrics Under Measurement
- [x] Response Time (primary: targeting >100% increase over baseline)
- [x] Throughput (deltas/sec — should remain normal; single PR)
- [x] Power Usage (on-target: sustained compute load, elevated expected)
- [x] Degradation Over Time (model: single PR, N runs)
- [x] Binary Size (.text grows from 7 new functions)
- [x] Runtime Performance (on-target: THIS is the primary measurement)

## Preconditions
- Baseline on `main` HEAD — no added workload.
- Clean build per Directive #3.
- Same hardware per Directive #5.
- Baseline response time recorded for `Blinky_LED.elf`.

## Test Procedure
1. Record baseline on `main` HEAD (`Blinky_LED.elf`).
2. Create branch `test/nested-loop-bloat_2026-02-27_10-00`.
3. Add `perf_bloat.c` with 7 functions as described above.
4. Modify `Cpu0_Main.c`: add `bloat_run_all()` call inside `while(1)`.
5. Build and verify no errors.
6. Commit, push, move test file to `Running/`, open PR.
7. GitHub Actions: clean build, record section sizes.
8. GitHub Actions: flash, run on-target timing (response time, throughput).
9. GitHub Actions: measure power draw.
10. Record model response time and throughput for processing the delta.
11. Repeat for N runs.
12. Compare all metrics against baseline.

## Input / Workload Description
- **Delta size:** Small file count (1 new file + 1 modified), but workload
  is computationally extreme per iteration.
- **Nature:** Additive — new file, new call in existing function.
- **Loop body cost (estimate):** `bloat_matrix32_multiply` alone performs
  32×32×32 = 32,768 multiply-add operations. At 300 MHz with one MAC per
  cycle, that is ~109 µs per call. Combined with the other 5 functions,
  total added loop body time is expected to exceed 200 µs, which is well
  above the ~6 µs baseline iteration time.

## Expected Results
- **Response time:** >100% increase (>2x baseline). The 32x32 matrix
  multiply alone exceeds the baseline iteration cost by a large margin.
  Conservative estimate: 10x–20x baseline. The test succeeds at any value
  ≥100% above baseline.
- **Binary .text size:** Moderate increase — 7 functions, none inlined.
  Estimated +2–4 KB.
- **Binary .data/.bss:** Slight increase from local array temporaries
  (if the compiler promotes them to static); otherwise unchanged.
- **Power usage:** Sustained elevated power. The multiply-accumulate units
  on TC397 will run at high utilization. Expected 20–50% increase over
  baseline power.
- **Model response time:** Normal — single-file delta is not large.
  Expected within 10% of PERF-001 model timing.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 7 | All noinline, all extern |
| New local/static symbols | 0 | No static functions |
| Inlined-away functions | 0 | noinline attribute on all |
| Modified existing functions | 1 | `core0_main` — adds `bloat_run_all()` in while(1) |
| Total function count delta | +7 | Baseline ~171 → expected ~178 |

**New function symbols (all `__attribute__((noinline))`, all GLOBAL):**
- `bloat_matrix32_multiply` — 32×32 integer matrix multiply
- `bloat_dot_product` — 256-element dot product
- `bloat_array_accumulate` — 512-element volatile sum
- `bloat_stride_scan` — 1024-element stride-7 scan
- `bloat_outer_product` — 16×16 outer product
- `bloat_nested_sum` — 16×16×4 triple-nested sum
- `bloat_run_all` — orchestrator, calls all above

**Inline vs Not-Inline:** Same pattern as PERF-001 — all noinline, all discrete
global symbols. Compares directly with PERF-001 but with heavier per-call cost.

## Source-to-Binary Function Correlation

```
Source                         Compilation                ELF Binary
─────                          ───────────                ──────────

perf_bloat.c                    GCC TriCore -O2
┌─────────────────────────┐    ┌──────────────┐    ┌──────────────────────────────┐
│ bloat_matrix32_multiply()│───▶│  noinline    │───▶│ bloat_matrix32_multiply [GLB]│
│ bloat_dot_product()      │───▶│  forces      │───▶│ bloat_dot_product       [GLB]│
│ bloat_array_accumulate() │───▶│  discrete    │───▶│ bloat_array_accumulate  [GLB]│
│ bloat_stride_scan()      │───▶│  symbol per  │───▶│ bloat_stride_scan       [GLB]│
│ bloat_outer_product()    │───▶│  function.   │───▶│ bloat_outer_product     [GLB]│
│ bloat_nested_sum()       │───▶│  No inlining │───▶│ bloat_nested_sum        [GLB]│
│ bloat_run_all()          │───▶│  possible.   │───▶│ bloat_run_all           [GLB]│
└─────────────────────────┘    └──────────────┘    │                              │
                                                    │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │
Cpu0_Main.c                                        │ core0_main            [GLB] │
┌─────────────────────────┐                        │   CALL bloat_run_all        │
│ core0_main()            │──── CALL ─────────────▶│   CALL blinkLED             │
│   while(1) {            │                        └──────────────────────────────┘
│     bloat_run_all() ◄───┼── IN LOOP (per iter)
│     blinkLED()          │
│   }                     │       7 new GLOBAL symbols
└─────────────────────────┘       0 inlined / invisible
                                  1 modified (core0_main)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Primary:** Response time must be >100% above baseline. If it isn't,
  the workload wasn't heavy enough and the test is redesigned, not failed.
- **Build:** Must compile and link cleanly.
- **Binary:** Section sizes recorded. `.text` must increase (7 new functions).
- **Model:** Must process without timeout or crash.
- **Power:** Increase is expected and acceptable. Spike >100% over baseline
  flags a review (thermal concern).

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test is the "control case" for the 100%+ degradation suite. The 32x32
matrix multiply is the bluntest instrument available — it adds a fixed, large,
predictable amount of work. If this doesn't achieve 100%+ degradation, nothing
in the suite will, and the baseline response time definition needs revisiting.

Pairs with PERF-007, PERF-008, and PERF-009 to compare different degradation
mechanisms. PERF-006 uses raw compute. PERF-007 uses memory latency. PERF-008
uses branch pressure. PERF-009 uses multi-core contention. Together they cover
the full TC397 bottleneck space.
