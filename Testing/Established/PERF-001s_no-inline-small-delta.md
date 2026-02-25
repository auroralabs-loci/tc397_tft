# Test Template

## Test ID
PERF-001s

## Test Name
No-Inline Small-Scale Delta

## Objective
Same concept as PERF-001 but scoped to a single file or a handful of
functions. Remove inline qualifiers from a small, targeted set of functions.
This measures the model's response to a minimal delta and gives us the
floor for inline-removal impact.

We want to answer:
- What is the minimum measurable impact of removing a few inlines?
- How fast does the model process a small, focused delta?
- Is the on-target difference even detectable at this scale?

## Target PR / Code Delta
- **Branch:** `test/no-inline-small-delta_<date_time>`
- **Change:** Remove `inline` / `__attribute__((always_inline))` from
  3–5 functions in a single file (e.g., `Blinky_LED.c` or `Multicore.c`).
- **Scope:** Small — 1 file, handful of edits.

## Metrics Under Measurement
- [x] Response Time (model: small delta, should be fast)
- [x] Throughput (model: baseline small-delta throughput)
- [x] Power Usage (on-target: likely undetectable at this scale)
- [x] Binary Size (small shift expected)
- [x] Runtime Performance (on-target: minimal change expected)

## Preconditions
- Baseline on `main` HEAD.
- Clean build, same hardware per Directives.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/no-inline-small-delta_<date_time>`.
3. Remove inline qualifiers from 3–5 functions in one file.
4. Commit, push, move to `Running/`, open PR.
5. GitHub Actions runs the full cycle.
6. Repeat for N runs.
7. Compare against baseline.

## Input / Workload Description
- **Delta size:** Small — 1 file, 3–5 function qualifier removals.
- **Nature:** Subtractive, minimal.

## Expected Results
- **Binary .text size:** Change of less than 1%. A few removed inlines
  won't meaningfully shift the total code size.
- **Binary .data/.bss:** No change.
- **Runtime performance:** Within noise of baseline (< 1% difference).
  A handful of de-inlined functions in non-critical paths won't produce
  a measurable timing shift.
- **Power usage:** Undetectable at this scale. Within measurement noise.
- **Model response time:** Fast — this is a trivial delta. Expected to
  complete in the model's minimum processing time.
- **Model throughput:** At or near peak — small deltas are the easy case.

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Build:** Must compile cleanly.
- **Model:** Must process quickly. This is the "easy case" — if the model
  struggles here, something is fundamentally wrong.
- **On-target:** Results recorded. No hard threshold — this establishes
  the noise floor for inline removal.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
Paired with PERF-001 (large). Comparing small vs large inline removal shows
how delta size scales model processing time and on-target impact.
