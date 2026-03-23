# Test Template

## Test ID
PERF-001

## Test Name
No-Inline Large-Scale Delta

## Objective
Measure the performance impact of removing all inline function qualifiers
across the codebase. Inline functions are expanded at the call site by the
compiler — removing them forces actual function calls with full call/return
overhead. This is a deliberately large change touching many files, so it
also serves as a stress test for the model's ability to process a big delta.

We want to answer:
- How much does removing inlining cost in execution speed?
- How does the binary size shift? (could shrink from fewer expansions or
  grow from added call overhead — we want to know which)
- Does the extra call/return activity show up in power draw?
- Can the performance model handle a PR of this scale without degrading?

## Target PR / Code Delta
- **Branch:** `test/no-inline-large-delta_<date_time>`
- **Change:** Remove `inline`, `__inline`, `__attribute__((always_inline))`,
  and any equivalent qualifiers from all function definitions across the
  project (Libraries, Configurations, blinky, multicore).
- **Scope:** Large — expected to touch most source and header files.

## Metrics Under Measurement
- [x] Response Time (how fast the model processes this delta)
- [x] Throughput (deltas processed per unit time under load)
- [x] Power Usage (chip power draw with non-inlined binary vs baseline)
- [x] Degradation Over Time (does model performance drop on repeated large deltas)
- [x] Memory Footprint (model memory consumption while processing)
- [x] Binary Size (section sizes: .text, .data, .bss — compare against baseline)
- [x] Runtime Performance (execution timing on-target for blinky and multicore)

## Preconditions
- Baseline measurements taken on current `main` HEAD with inlines intact.
- Clean build from scratch (Directive #3).
- Same hardware setup as baseline (Directive #5).
- GitHub Actions pipeline configured and operational.

## Test Procedure
1. Record baseline: build `main` HEAD as-is, flash, measure all metrics.
2. Create branch `test/no-inline-large-delta_<date_time>` from `main`.
3. Strip all inline qualifiers from every source and header file in the project.
4. Commit the change, push the branch.
5. Move this test file to `Testing/Running/`.
6. Open PR against `main` — GitHub Actions picks it up.
7. CI builds the binary (clean build), records binary section sizes.
8. CI flashes and runs on-target measurements (timing, power).
9. Model processes the PR delta — record response time, throughput, memory.
10. Repeat steps 7–9 for N runs (minimum per Directive #7).
11. Collect results, compare against baseline.

## Input / Workload Description
- **Delta size:** Large — removal of inline qualifiers across all files.
  Expected to touch 20+ files with hundreds of individual edits.
- **Nature:** Purely subtractive (removing qualifiers), no logic changes.
- **Binary impact:** Function call overhead replaces inline expansion at
  every former inline call site.

## Expected Results
- **Binary .text size:** Likely decreases (fewer inline expansions) but
  call stubs add some overhead — net direction to be determined.
- **Runtime performance:** Expected to degrade — more function calls means
  more stack operations, pipeline stalls, and potential cache pressure on
  the TriCore.
- **Power usage:** Expected to increase slightly due to more memory accesses
  and instruction fetches from call/return sequences.
- **Model response time:** Expected to be higher than small-delta tests due
  to the volume of changes to process.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 7 | All workload functions forced as discrete symbols via `__attribute__((noinline))` |
| New local/static symbols | 0 | No static functions in this variant |
| Inlined-away functions | 0 | `noinline` attribute prevents all inlining |
| Modified existing functions | 1 | `core0_main` — adds call to `perf_run_all_workloads()` in main loop |
| Total function count delta | +7 | Baseline ~171 functions → expected ~178 |

**New function symbols (all `__attribute__((noinline))`, all GLOBAL):**
- `perf_matrix_multiply` — 8x8 matrix multiply
- `perf_crc32_compute` — bit-by-bit CRC32
- `perf_bubble_sort` — bubble sort 256 elements
- `perf_memory_stress` — volatile memory read/write
- `perf_bitfield_stress` — shift/rotate/XOR operations
- `perf_fibonacci_iterative` — iterative fibonacci(40)
- `perf_run_all_workloads` — orchestrator, calls all above

**Inline vs Not-Inline:** All 7 functions are explicitly `noinline`. The compiler
MUST emit a separate symbol for each, with full call/return overhead at every call
site. This is the maximum-overhead variant — paired with PERF-005 which makes the
same functions `always_inline`.

## Source-to-Binary Function Correlation

```
Source                          Compilation                    ELF Binary
─────                          ───────────                    ──────────

perf_workload_noinline.c        GCC TriCore -O2
┌─────────────────────────┐    ┌──────────────┐    ┌─────────────────────────────┐
│ perf_matrix_multiply()  │───▶│  noinline    │───▶│ perf_matrix_multiply  [GLB] │
│ perf_crc32_compute()    │───▶│  Forces       │───▶│ perf_crc32_compute    [GLB] │
│ perf_bubble_sort()      │───▶│  discrete     │───▶│ perf_bubble_sort      [GLB] │
│ perf_memory_stress()    │───▶│  symbol       │───▶│ perf_memory_stress    [GLB] │
│ perf_bitfield_stress()  │───▶│  emission.    │───▶│ perf_bitfield_stress  [GLB] │
│ perf_fibonacci_iter..() │───▶│  Each gets    │───▶│ perf_fibonacci_iter.. [GLB] │
│ perf_run_all_workloads()│───▶│  own entry.   │───▶│ perf_run_all_workloads[GLB] │
└─────────────────────────┘    └──────────────┘    │                             │
                                                    │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ │
Cpu0_Main.c                                        │ core0_main            [GLB] │
┌─────────────────────────┐                        │   CALL perf_run_all_wklds   │
│ core0_main()            │───────── CALL ────────▶│   CALL blinkLED             │
│   while(1) {            │                        └─────────────────────────────┘
│     perf_run_all_wklds()│
│     blinkLED()          │        7 new GLOBAL symbols
│   }                     │        0 inlined / invisible
└─────────────────────────┘        1 modified (core0_main)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Model processing:** Must complete without timeout or crash. Response
  time and throughput recorded for benchmarking (no hard pass/fail yet —
  this is a characterization test).
- **Build:** Must compile and link successfully with no errors.
- **Binary:** Section sizes must be recorded and compared to baseline.
- **Runtime:** On-target timing must be recorded. Any regression >___% to
  be flagged (threshold TBD with user).

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test doubles as both a performance characterization (what does removing
inlines actually cost on TC397?) and a model stress test (can it handle a
large-scale PR without choking?). The two concerns are measured independently.
