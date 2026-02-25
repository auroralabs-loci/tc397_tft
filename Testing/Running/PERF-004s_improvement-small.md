# Test Template

## Test ID
PERF-004s

## Test Name
Small Improvement Delta

## Objective
A focused, minimal optimization PR. Instead of optimizing the whole
codebase (PERF-004), we make one or two targeted improvements to a single
file. This tests whether the model detects a small positive change and
whether the on-target gain is measurable.

We want to answer:
- Can the model detect a small improvement in a minimal delta?
- Is a single-file optimization measurable on-target?
- How does model processing time compare to other small deltas?

## Target PR / Code Delta
- **Branch:** `test/improvement-small_<date_time>`
- **Change:** Apply 1–2 targeted optimizations in a single file. Examples:
  - Replace a busy-wait loop with a timer check.
  - Use a TriCore intrinsic instead of a multi-instruction C sequence.
  - Optimize a frequently-called function's memory access pattern.
- **Scope:** Small — 1 file, 1–2 changes.

## Metrics Under Measurement
- [x] Response Time (model: small delta, quick processing)
- [x] Throughput (model: baseline small-delta throughput)
- [x] Power Usage (on-target: detectable if change hits a hot path)
- [x] Binary Size (small shift expected)
- [x] Runtime Performance (on-target: measurable if targeting hot code)

## Preconditions
- Baseline on `main` HEAD.
- Clean build, same hardware per Directives.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/improvement-small_<date_time>`.
3. Apply 1–2 targeted optimizations in one file.
4. Verify functional equivalence.
5. Commit, push, move to `Running/`, open PR.
6. GitHub Actions runs full cycle.
7. Repeat for N runs.
8. Compare against baseline.

## Input / Workload Description
- **Delta size:** Small — 1 file, 1–2 changes.
- **Nature:** Optimization — tighter code, same behavior.

## Expected Results
- **Binary .text size:** Decrease of 0.1–1%. A single intrinsic swap
  or loop optimization trims a small amount of code.
- **Binary .data/.bss:** No change.
- **Runtime performance:** If the optimized function is in a hot path,
  improvement of 1–5%. If cold path, within noise of baseline. This
  depends entirely on which function is targeted.
- **Power usage:** Marginal decrease if a hot loop is tightened. Likely
  within measurement noise for a single cold-path optimization.
- **Model response time:** Fast — trivial delta size. Should be at or
  near the model's minimum processing time.
- **Model throughput:** At peak — this is the easiest case.

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Build:** Must compile cleanly.
- **Functional:** Must behave identically to baseline.
- **Model:** Must process quickly and correctly.
- **On-target:** Results recorded. No hard threshold — this is the
  small-scale data point for the improvement curve.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
Paired with PERF-004 (large). Small vs large improvement comparison shows
how optimization gains scale and whether the model's detection sensitivity
has a lower bound.
