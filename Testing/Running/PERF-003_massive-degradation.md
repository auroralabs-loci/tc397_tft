# Test Template

## Test ID
PERF-003

## Test Name
Massive Degradation Stress Test

## Objective
Push the performance model to its limits and find out where it breaks down.
This isn't about one PR — it's about flooding the system with a sustained,
heavy stream of large deltas and watching how the metrics degrade over time.

We want to answer:
- At what point does the model start slowing down?
- Is the degradation gradual (linear) or does it hit a cliff?
- Does the model recover after the load stops, or does damage persist?
- What are the actual breaking points for response time and throughput?
- Do on-target measurements drift when the system is under sustained stress?

## Target PR / Code Delta
- **Branch pattern:** `test/degradation-round-XX_<date_time>` (one branch
  per round)
- **Change:** A series of increasingly heavy PRs submitted in rapid
  succession. Each round escalates:
  - **Round 1:** Small deltas — single-file, few-line changes. Warm-up.
  - **Round 2:** Medium deltas — multi-file refactors, function moves.
  - **Round 3:** Large deltas — sweeping changes across the codebase
    (similar in scale to PERF-001).
  - **Round 4:** Overlapping PRs — multiple large deltas open at the
    same time. Maximum concurrent load.
  - **Round 5:** Recovery — stop submitting. Measure how long it takes
    for response times to return to baseline (if they do).
- **Scope:** Massive — this is a sustained campaign, not a single test.

## Metrics Under Measurement
- [x] Response Time (per delta, tracked over time — looking for the curve)
- [x] Throughput (deltas/sec at each round — looking for the drop-off)
- [x] Power Usage (on-target: does sustained activity affect chip thermals?)
- [x] Degradation Over Time (this is the primary metric — the whole point)
- [x] Memory Footprint (does the model leak memory under sustained load?)
- [x] Recovery Time (how long after load stops to return to baseline)
- [x] Error Rate (does the model start producing errors under pressure?)

## Preconditions
- Baseline taken at rest — model idle, single clean build/flash cycle.
- All prior tests completed or paused (no background noise).
- GitHub Actions runners available and not throttled.
- Monitoring in place for model memory, CPU, and response times before
  the test begins.

## Test Procedure
1. Record baseline: model at idle, process one small delta, measure
   response time and throughput.
2. **Round 1 (warm-up):** Submit 5 small-delta PRs spaced 1 minute apart.
   Record response time for each.
3. **Round 2 (medium load):** Submit 5 medium-delta PRs spaced 30 seconds
   apart. Record response time, throughput, memory.
4. **Round 3 (heavy load):** Submit 5 large-delta PRs spaced 15 seconds
   apart. Record all metrics.
5. **Round 4 (overload):** Open 5+ large-delta PRs simultaneously. Record
   all metrics. Watch for timeouts, errors, queuing behavior.
6. **Round 5 (recovery):** Stop all submissions. Measure response time
   every minute for 15 minutes. Plot the recovery curve.
7. At each round, also run one on-target flash + measurement cycle to
   check whether hardware-side results remain consistent under CI load.
8. Compile all metrics into a time-series for analysis.

## Input / Workload Description
- **Total deltas:** ~20–25 PRs across all rounds.
- **Delta sizes:** Escalating from single-file tweaks to full-codebase
  sweeps.
- **Timing:** Deliberately compressed — the goal is to overwhelm, not
  to give breathing room.
- **Duration:** Entire test campaign expected to take 30–60 minutes
  depending on model response times.

## Expected Results
- **Round 1–2:** Response times close to baseline. No degradation.
- **Round 3:** Response times start climbing. Throughput begins to drop.
- **Round 4:** Significant degradation. Possible timeouts or queuing.
  Memory usage elevated.
- **Round 5:** Gradual recovery. If memory leaks exist, recovery will be
  incomplete.
- **On-target:** Hardware measurements should remain stable regardless of
  model load (the chip doesn't know or care how busy the CI is). If
  hardware results drift, something else is wrong.

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
This is a characterization test — the goal is to find limits, not to pass.
However:
- **Hard fail:** Model crashes, produces corrupted output, or fails to
  recover within 15 minutes after load stops.
- **Soft fail:** Response time exceeds ___x baseline (threshold TBD).
  Throughput drops below ___% of baseline (threshold TBD).
- **Pass:** Model degrades gracefully, recovers fully, no data loss.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This is the most important test in the suite. The other tests tell us how
the model handles specific kinds of changes. This one tells us where it
falls apart. Every system has a breaking point — the goal here is to find
it under controlled conditions rather than discovering it in production.

The round structure makes results easy to graph: X axis is time/load
intensity, Y axis is response time or throughput. We're looking for the
knee in the curve.
