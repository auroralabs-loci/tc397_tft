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
