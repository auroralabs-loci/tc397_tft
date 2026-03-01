# Test Template

## Test ID
PERF-008

## Test Name
Branch Prediction Destroyer — Data-Dependent Pipeline Stalls

## Objective
Achieve 100%+ response time degradation by maximizing branch mispredictions.
Unlike PERF-006 (compute-bound) and PERF-007 (memory-latency-bound), this
test creates pathological branching patterns where the TC397's branch predictor
cannot build a useful prediction table. Every conditional outcome depends on
runtime data that changes unpredictably.

We want to answer:
- How much of TC397's response time is attributable to branch misprediction
  penalties rather than instruction throughput or memory latency?
- Does the TriCore architecture's pipeline flush cost make branch-heavy code
  disproportionately expensive?
- Can we achieve 100%+ degradation with a small binary size footprint
  (few instructions, all branches)?
- Does the analysis model correctly distinguish branch-heavy code from
  arithmetic-heavy code in its commentary?

## Target PR / Code Delta
- **Branch:** `test/branch-prediction-destroyer_2026-02-27_10-30`
- **Change:** Add `perf_branch.c` with 7 functions, all `noinline`,
  called from `core0_main`'s `while(1)` via an orchestrator. Each
  function is specifically designed to defeat branch prediction:
  - `branch_data_dependent_sort` — sort a 256-element array of
    pseudo-random values using selection sort. Each comparison is a
    data-dependent branch. Array is re-scrambled after each sort using
    a simple LCG seeded from the sort result, so the branch sequence
    changes every iteration.
  - `branch_threshold_cascade` — 512 elements, each checked against 4
    different thresholds in nested if-else chains. Threshold values are
    derived from the previous element's outcome, creating a dependency
    chain that defeats both the predictor and out-of-order speculation.
  - `branch_bit_scatter` — 64 iterations; each iteration examines 8
    bits of a 32-bit value and branches based on each bit independently.
    The 8-bit pattern changes each iteration using an LFSR.
  - `branch_search_unsorted` — linear search through a 512-element
    unsorted volatile array for a target that moves each call. Branch
    taken/not-taken ratio is 1:511 on average but varies.
  - `branch_early_exit_sabotage` — a function designed to look like it
    should early-exit but never does. 128 iterations, each with 3
    conditions that individually trigger often but never simultaneously.
  - `branch_nested_dispatch` — a 4-level deep switch-case dispatch tree
    where each level's selector depends on the result of the level above.
    Each level has 8 cases. Total combinations: 4096 paths, uniformly
    distributed.
  - `branch_run_all` — orchestrator, calls all six above.
- **Scope:** 1 new `.c` file + 1 modified caller.

## Metrics Under Measurement
- [x] Response Time (primary: targeting >100% increase, branch-miss driven)
- [x] Throughput (model: single PR, normal expected)
- [x] Power Usage (on-target: pipeline flushes burn power on refetch)
- [x] Degradation Over Time (model: N runs)
- [x] Binary Size (.text grows — branch-heavy code tends to be larger per
  function than arithmetic-heavy code due to conditional jump sequences)
- [x] Runtime Performance (on-target: every misprediction costs 5–9 pipeline
  stages on TC397)

## Preconditions
- Baseline on `main` HEAD.
- Clean build per Directive #3.
- Same hardware per Directive #5.
- Baseline response time recorded.

## Test Procedure
1. Record baseline on `main` HEAD (`Blinky_LED.elf`).
2. Create branch `test/branch-prediction-destroyer_2026-02-27_10-30`.
3. Add `perf_branch.c`.
4. Modify `Cpu0_Main.c`: add `branch_run_all()` inside `while(1)`.
5. Build and verify cleanly.
6. Commit, push, move test file to `Running/`, open PR.
7. GitHub Actions: clean build, record section sizes.
8. GitHub Actions: flash, on-target timing and power.
9. Record model metrics.
10. Repeat for N runs.
11. Compare against baseline.

## Input / Workload Description
- **Delta size:** Small file count (1 new + 1 modified).
- **Workload per iteration:** Each function creates a high-entropy branch
  sequence. Key costs:
  - `branch_data_dependent_sort`: 256 elements × avg 128 comparisons each
    (selection sort) = ~32,768 conditional branches per call.
  - `branch_threshold_cascade`: 512 × 4 conditions = 2,048 branches.
  - `branch_bit_scatter`: 64 × 8 = 512 branches.
  - Total: ~35,000+ conditional branches per iteration, each with
    unpredictable outcome (data-dependent from previous computation).
- **Nature:** Additive — new file, new call.

## Expected Results
- **Response time:** >100% increase expected from misprediction penalties.
  TC397 pipeline flush on misprediction costs approximately 5–9 cycles.
  35,000 mispredictions × 7 cycles avg = 245,000 wasted cycles per
  iteration. At 300 MHz, that is ~817 µs added per pass — far above the
  ~6 µs baseline. Actual results depend heavily on prediction hit rate;
  even 50% prediction accuracy would yield massive degradation.
- **Binary .text size:** Medium increase — branch-heavy functions tend to
  be compact in instruction count but generate many jump instructions.
  Estimated +2–4 KB.
- **Binary .data/.bss:** Minimal — the arrays used are local (stack) or
  small static. Estimated +0–512 bytes.
- **Power usage:** Elevated. Pipeline flushes force re-fetching instructions
  from the cache/flash repeatedly. Expected 15–40% increase over baseline.
- **Model response time:** Normal — small source delta.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 7 | All noinline, all extern |
| New local/static symbols | 0 | No static functions |
| Inlined-away functions | 0 | noinline prevents all inlining |
| Modified existing functions | 1 | `core0_main` — adds `branch_run_all()` in while(1) |
| Total function count delta | +7 | Baseline ~171 → expected ~178 |

**New function symbols (all `__attribute__((noinline))`, all GLOBAL):**
- `branch_data_dependent_sort` — scramble-sort cycle, 256 elements
- `branch_threshold_cascade` — 4-threshold nested conditionals, 512 elements
- `branch_bit_scatter` — 8 branches per 32-bit word, 64 words
- `branch_search_unsorted` — linear search, moving target, 512 elements
- `branch_early_exit_sabotage` — pathological never-exits early loop
- `branch_nested_dispatch` — 4-deep, 8-wide switch dispatch tree
- `branch_run_all` — orchestrator, calls all six above

## Source-to-Binary Function Correlation

```
Source                         Compilation                ELF Binary
─────                          ───────────                ──────────

perf_branch.c                   GCC TriCore -O2
┌───────────────────────────┐  ┌──────────────┐    ┌──────────────────────────────┐
│ branch_data_dep_sort()    │─▶│  noinline    │───▶│ branch_data_dependent_sort[G]│
│ branch_threshold_cascade()│─▶│  all extern  │───▶│ branch_threshold_cascade  [G]│
│ branch_bit_scatter()      │─▶│  discrete    │───▶│ branch_bit_scatter        [G]│
│ branch_search_unsorted()  │─▶│  symbols.    │───▶│ branch_search_unsorted    [G]│
│ branch_early_exit_sab()   │─▶│  Data-dep.   │───▶│ branch_early_exit_sabotage[G]│
│ branch_nested_dispatch()  │─▶│  branches    │───▶│ branch_nested_dispatch    [G]│
│ branch_run_all()          │─▶│  defeat pred.│───▶│ branch_run_all            [G]│
└───────────────────────────┘  └──────────────┘    │                              │
                                                    │ core0_main            [GLB] │
Cpu0_Main.c                                        │   CALL branch_run_all        │
┌───────────────────────────┐                      │   CALL blinkLED              │
│ core0_main()              │──── CALL ───────────▶└──────────────────────────────┘
│   while(1) {              │
│     branch_run_all() ◄────┼── EVERY ITERATION
│     blinkLED()            │
│   }                       │        7 new GLOBAL symbols
└───────────────────────────┘        0 inlined / invisible
                                     1 modified (core0_main)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Primary:** Response time must be >100% above baseline.
- **Determinism:** The LCG/LFSR seeds must be constants (not time-based)
  so that the branch sequence is deterministic across runs. Results must
  be reproducible (Directive #7). If variance between runs exceeds 5%,
  investigate seed dependency.
- **Build:** Must compile and link cleanly.
- **Model:** Must process without timeout or crash.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test is the third vertex of the degradation triangle alongside PERF-006
(compute) and PERF-007 (memory latency). Comparing the three tells us which
bottleneck is the TC397's Achilles heel.

The `branch_data_dependent_sort` function's re-scramble step is critical:
without it, the predictor would learn the sort's branch pattern within a few
iterations and eliminate the misprediction cost. The re-scramble ensures
the branch sequence is always novel.

If the compiler turns any of these into branchless CMOV equivalents,
the test degrades gracefully (computes the same results, just faster).
Document any such compiler optimizations — they're informative about GCC's
TriCore backend behavior.
