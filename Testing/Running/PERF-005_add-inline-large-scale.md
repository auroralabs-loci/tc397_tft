# Test Template

## Test ID
PERF-005

## Test Name
Add Inline Functions — Large Scale

## Objective
The mirror of PERF-001. Instead of removing inlines, we aggressively add
`inline` qualifiers to every eligible function across the codebase. This
forces the compiler to expand function bodies at every call site, trading
binary size for call overhead elimination.

We want to answer:
- How much does aggressive inlining improve (or hurt) runtime on TC397?
- What's the binary size cost of inlining everything?
- At what point does inlining become counterproductive (instruction cache
  pressure, binary bloat)?
- Can the model handle another large-scale delta efficiently?

## Target PR / Code Delta
- **Branch:** `test/add-inline-large_<date_time>`
- **Change:** Add `static inline` or `__attribute__((always_inline))` to
  every function that qualifies (non-recursive, defined in headers or
  translation units where the compiler can see the body). Apply across
  all project files: Libraries, Configurations, blinky, multicore.
- **Scope:** Large — similar breadth to PERF-001 but in the opposite
  direction.

## Metrics Under Measurement
- [x] Response Time (model processing time for a large additive delta)
- [x] Throughput (model deltas/sec)
- [x] Power Usage (on-target: inlined code may reduce or increase power)
- [x] Degradation Over Time (model: single PR, recorded for comparison)
- [x] Binary Size (.text expected to grow significantly)
- [x] Runtime Performance (on-target: fewer function calls vs cache pressure)

## Preconditions
- Baseline on `main` HEAD (normal inline usage).
- Clean build, same hardware per Directives.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/add-inline-large_<date_time>`.
3. Add inline qualifiers to all eligible functions.
4. Build and verify no errors or warnings from over-inlining.
5. Commit, push, move test file to `Running/`, open PR.
6. GitHub Actions: clean build, record section sizes.
7. GitHub Actions: flash, run on-target measurements.
8. Record model metrics.
9. Repeat for N runs.
10. Compare against baseline.

## Input / Workload Description
- **Delta size:** Large — every eligible function across the project.
- **Nature:** Additive — adding qualifiers, not changing logic.
- **File count:** 20+ files expected.

## Expected Results
- **Binary .text size:** Increase of 15–40%. Every inline expansion
  duplicates the function body at the call site. Functions called from
  multiple places will balloon the code section.
- **Binary .data/.bss:** No change expected — inlining doesn't affect
  data allocation.
- **Runtime performance:** Small improvement (2–10%) for functions with
  few call sites. Possible regression for heavily-called functions due
  to instruction cache pressure — the expanded code may not fit in cache
  as well as the non-inlined version. Net result depends on the call
  graph balance.
- **Power usage:** Mixed. Fewer function calls = less stack activity =
  less power. But larger code footprint = more instruction fetches from
  flash = more power. Expect roughly flat, within +/- 5% of baseline.
- **Model response time:** Comparable to PERF-001 — similar delta size,
  different direction. Expected within 10% of PERF-001 model timing.
- **Model throughput:** No significant difference from other large-delta
  tests.

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Build:** Must compile and link. Over-inlining may cause warnings —
  document them but they don't fail the test.
- **Binary size:** Growth is expected and acceptable. Exceeding 50%
  growth in .text flags a review.
- **Runtime:** Record and compare. Regression >15% = investigate.
- **Power:** Must not increase more than 10% over baseline.
- **Model:** Must process without timeout or crash.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This is the direct inverse of PERF-001. Comparing PERF-001 (remove inlines)
vs PERF-005 (add inlines) vs baseline gives us a three-point curve for
inline impact on TC397. The truth is usually in the middle — some functions
benefit from inlining, others don't. These extreme tests bracket the range.
