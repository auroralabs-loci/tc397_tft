# Test Template

## Test ID
PERF-002s

## Test Name
Assembly Block Delta — Small Scale

## Objective
Same concept as PERF-002 but limited to a single function replaced with
inline assembly. Tests whether the model handles a small asm-containing
delta cleanly, and gives us the baseline cost of asm in a minimal change.

We want to answer:
- Does even a single asm block affect model processing differently than C?
- What is the on-target impact of one hand-written asm function?

## Target PR / Code Delta
- **Branch:** `test/assembly-blocks-small_<date_time>`
- **Change:** Replace the body of one function with equivalent inline
  assembly. Pick a function with clear, measurable behavior (e.g., a
  delay loop, a register configuration routine).
- **Scope:** Small — 1 function, 1 file.

## Metrics Under Measurement
- [x] Response Time (model: minimal delta with asm content)
- [x] Throughput (model: baseline)
- [x] Power Usage (on-target: single function — likely negligible)
- [x] Binary Size (small shift from compiler vs hand asm difference)
- [x] Runtime Performance (on-target: one function, measurable if it's hot)

## Preconditions
- Baseline on `main` HEAD.
- Assembly must be valid TriCore TC1.6.2.
- Clean build, same hardware per Directives.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/assembly-blocks-small_<date_time>`.
3. Replace one function body with equivalent inline asm.
4. Verify functional equivalence.
5. Commit, push, move to `Running/`, open PR.
6. GitHub Actions runs full cycle.
7. Repeat for N runs.
8. Compare against baseline.

## Input / Workload Description
- **Delta size:** Small — 1 function in 1 file.
- **Nature:** Replacement — C body swapped for asm.

## Expected Results
- **Binary .text size:** Change of a few bytes to a few dozen bytes.
  Direction depends on whether the hand-written asm is tighter or
  looser than what the compiler generated.
- **Binary .data/.bss:** No change.
- **Runtime performance:** If the function is in a hot path, measurable
  improvement of 1–5% is possible with good asm. If it's a cold path,
  within noise of baseline.
- **Power usage:** Undetectable for a single function unless it's a
  tight inner loop. Within measurement noise.
- **Model response time:** Fast — small delta. The asm content should
  add minimal overhead vs an equivalent C-only small delta.
- **Model throughput:** At or near peak.

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Build:** Must compile. Asm syntax errors = blocked, not failed.
- **Functional:** Must behave identically to baseline.
- **Model:** Must process cleanly and quickly.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
Paired with PERF-002 (large/medium). The small variant isolates whether
asm content itself causes model overhead vs the volume of asm changes.
