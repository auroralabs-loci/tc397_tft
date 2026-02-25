# Test Template

## Test ID
PERF-003s

## Test Name
Degradation Test — Small Scale

## Objective
A lighter version of PERF-003. Instead of flooding the model with massive
deltas, we send a sustained stream of small, trivial PRs. This tests
whether volume alone (many small changes) causes degradation, even when
each individual delta is easy to process.

We want to answer:
- Does the model degrade under high volume of small deltas?
- Is degradation driven by delta size, delta count, or both?
- Where is the throughput ceiling for small changes?

## Target PR / Code Delta
- **Branch pattern:** `test/degradation-small-round-XX_<date_time>`
- **Change:** Rapid succession of small PRs:
  - **Round 1:** 10 trivial PRs (rename a variable, change a constant).
  - **Round 2:** 20 trivial PRs, faster cadence.
  - **Round 3:** 50 trivial PRs, maximum submission rate.
  - **Round 4:** Recovery — stop and observe.
- **Scope:** Each delta is tiny. The stress comes from quantity and speed.

## Metrics Under Measurement
- [x] Response Time (per delta, tracked across all rounds)
- [x] Throughput (deltas/sec — looking for the ceiling)
- [x] Degradation Over Time (primary metric)
- [x] Memory Footprint (does high volume cause memory growth?)
- [x] Recovery Time (return to baseline after stop)
- [x] Error Rate (any processing failures under volume?)

## Preconditions
- Baseline at idle with a single small delta.
- All other tests paused.
- GitHub Actions runners available.

## Test Procedure
1. Record baseline: one small delta, measure response time.
2. Execute rounds 1–3 with increasing volume and speed.
3. Record response time for every single delta.
4. After round 3, stop. Measure recovery over 10 minutes.
5. Plot response time vs delta count.

## Input / Workload Description
- **Total deltas:** ~80 across all rounds.
- **Delta size:** Trivial — 1–3 lines each.
- **Timing:** Compressed — testing volume, not complexity.

## Expected Results
- **Round 1 (10 PRs):** Response times steady, near baseline. No
  degradation. Throughput at peak. The model handles this easily.
- **Round 2 (20 PRs):** Response times may start creeping up 5–10%
  toward the end. Throughput still high but approaching saturation.
- **Round 3 (50 PRs):** Queuing becomes visible. Response times climb
  20–50% as backlog builds. Throughput plateaus — the model can't
  process faster than its pipeline allows. Memory may start growing
  if requests are queued in memory.
- **Round 4 (recovery):** Response times drop back to baseline within
  2–5 minutes. If memory was growing, it should stabilize and
  eventually return to baseline. If it doesn't, there's a leak.
- **Error rate:** Zero errors expected for small deltas even under
  volume. If errors appear, the model has a concurrency issue.

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Hard fail:** Any processing errors on trivial deltas. Memory that
  doesn't recover after load stops.
- **Soft fail:** Response time exceeds 2x baseline during round 2
  (too early for degradation on small deltas).
- **Pass:** Graceful throughput plateau, full recovery, zero errors.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
Paired with PERF-003 (massive). Comparing the two reveals whether
degradation is driven by delta size (003), delta volume (003s), or both.
