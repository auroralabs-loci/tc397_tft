# Test Template

## Test ID
PERF-004

## Test Name
Massive Improvement Delta

## Objective
The opposite of PERF-003. Instead of stressing the system with bad changes,
we submit a PR that makes everything significantly better — aggressive
optimizations across the entire codebase. This tests whether the model
recognizes and correctly reports large-scale improvements, and whether the
on-target binary actually delivers the expected gains.

We want to answer:
- Does the model accurately detect and report a massive positive delta?
- Are the on-target improvements real and measurable?
- Does the model process a large "improvement" delta differently than a
  large "degradation" delta? (It shouldn't — same workload either way.)
- How do binary size, runtime, and power shift when the codebase is
  aggressively optimized?

## Target PR / Code Delta
- **Branch:** `test/massive-improvement_<date_time>`
- **Change:** Apply sweeping optimizations across the codebase:
  - Replace busy-wait loops with timer-based waits where possible.
  - Optimize memory access patterns (align structures, reduce padding).
  - Convert frequently-called small functions to `static inline`.
  - Use TriCore-specific intrinsics for operations currently done in
    generic C (e.g., saturated arithmetic, bit manipulation).
  - Enable link-time optimization (LTO) if not already active.
  - Remove dead code paths and unused variables.
- **Scope:** Large — touches most files, but every change is an improvement.

## Metrics Under Measurement
- [x] Response Time (model processing: large delta, should be similar to PERF-001)
- [x] Throughput (model: no reason this should differ from other large deltas)
- [x] Power Usage (on-target: optimized code should draw less power)
- [x] Degradation Over Time (model: single large PR, not sustained — N/A for model)
- [x] Binary Size (.text should shrink from dead code removal and tighter code)
- [x] Runtime Performance (on-target: this should measurably improve)

## Preconditions
- Baseline on `main` HEAD (unoptimized).
- All optimizations must be functionally equivalent — same observable
  behavior as baseline.
- Clean build, same hardware, same everything per Directives.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/massive-improvement_<date_time>`.
3. Apply all optimizations listed above.
4. Verify functional equivalence (same outputs, same behavior).
5. Commit, push, move test file to `Running/`, open PR.
6. GitHub Actions: clean build, record section sizes.
7. GitHub Actions: flash, run on-target timing and power measurements.
8. Record model response time and throughput.
9. Repeat for N runs.
10. Compare all metrics against baseline.

## Input / Workload Description
- **Delta size:** Large — similar file count to PERF-001.
- **Nature:** Additive and subtractive — adding optimizations, removing
  waste. Net change is improvement across the board.

## Expected Results
- **Binary .text size:** Decrease of 5–15%. Dead code removal and tighter
  intrinsics should shrink the code section noticeably.
- **Binary .data/.bss:** Slight decrease from structure realignment and
  removal of unused globals.
- **Runtime performance:** Improvement of 10–30%. Timer-based waits alone
  free up significant CPU time. Intrinsics replace multi-instruction C
  sequences with single TriCore instructions.
- **Power usage:** Decrease of 5–20%. Less CPU activity, fewer memory
  accesses, tighter loops all reduce dynamic power.
- **Model response time:** Similar to other large-delta tests (PERF-001).
  The model shouldn't care whether the delta is "good" or "bad" — it's
  the same volume of changes to process.
- **Model throughput:** No difference expected from baseline large-delta
  throughput.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 7 | All optimized workload functions, extern |
| New local/static symbols | 0 | CRC table is `static const` data, not a function |
| Inlined-away functions | 0 | No inline qualifiers — optimizations are algorithmic |
| Modified existing functions | 1 | `core0_main` — adds call to `opt_run_all()` in main loop |
| Total function count delta | +7 | Baseline ~171 → expected ~178 |

**New function symbols (all extern, all GLOBAL):**
- `opt_matrix_multiply` — cache-friendly transposed 8x8 multiply
- `opt_crc32_table` — 256-entry lookup table CRC
- `opt_insertion_sort` — insertion sort (vs bubble sort in PERF-001)
- `opt_memory_copy_aligned` — 4-element unrolled memcpy
- `opt_bitfield_fast` — optimized bitfield operations
- `opt_fibonacci` — iterative fibonacci with early exit
- `opt_run_all` — orchestrator, calls all above

**Static data:** `crc_table[256]` precomputed CRC32 LUT in `.rodata` (+1KB).

**Inline vs Not-Inline:** None inline. Same 7-function structure as PERF-001
but with algorithmic optimizations (LUT CRC, insertion sort, loop unrolling).

## Source-to-Binary Function Correlation

```
Source                         Compilation              ELF Binary
─────                          ───────────              ──────────

perf_optimized.c                GCC TriCore -O2
┌───────────────────────┐      ┌──────────────┐    ┌──────────────────────────────┐
│ opt_matrix_multiply() │─────▶│  All extern  │───▶│ opt_matrix_multiply    [GLB] │
│ opt_crc32_table()     │─────▶│  discrete    │───▶│ opt_crc32_table        [GLB] │
│ opt_insertion_sort()  │─────▶│  symbols     │───▶│ opt_insertion_sort     [GLB] │
│ opt_memory_copy_aln() │─────▶│              │───▶│ opt_memory_copy_aligned[GLB] │
│ opt_bitfield_fast()   │─────▶│  Optimized   │───▶│ opt_bitfield_fast      [GLB] │
│ opt_fibonacci()       │─────▶│  algorithms  │───▶│ opt_fibonacci          [GLB] │
│ opt_run_all()         │─────▶│              │───▶│ opt_run_all            [GLB] │
└───────────────────────┘      └──────────────┘    │                              │
                                                    │ crc_table[256] → .rodata     │
Cpu0_Main.c                                        │                              │
┌───────────────────────┐                          │ core0_main             [GLB] │
│ core0_main()          │──── CALL ───────────────▶│   CALL opt_run_all           │
│   while(1) {          │                          │   CALL blinkLED              │
│     opt_run_all()     │                          └──────────────────────────────┘
│     blinkLED()        │
│   }                   │         7 new GLOBAL symbols + 1KB .rodata
└───────────────────────┘         1 modified (core0_main)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Build:** Must compile and link cleanly.
- **Functional equivalence:** Optimized binary must produce identical
  observable behavior to baseline. Any difference = fail.
- **Binary size:** Must decrease or stay flat. An increase indicates
  an "optimization" that isn't.
- **Runtime:** Must show measurable improvement (>5% vs baseline).
  No improvement or a regression = fail.
- **Power:** Must not increase. Flat is acceptable, decrease is expected.
- **Model:** Must process without timeout or crash.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test pairs with PERF-001 (degradation via inline removal) and PERF-003
(sustained degradation). Together they form a triangle: one makes things
worse, one makes things better, one finds the breaking point. Comparing
model behavior across all three tells us whether the model treats
improvements and regressions symmetrically.
