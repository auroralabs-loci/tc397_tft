# Test Template

## Test ID
PERF-007

## Test Name
Pointer Chase Labyrinth — Cache-Miss Induced Degradation

## Objective
Achieve 100%+ response time degradation by exploiting memory latency rather
than raw compute. Linked list traversal through a large volatile node pool
forces one cache miss per step — there is no prefetch-friendly access pattern
to exploit. The TC397's cache hierarchy is bypassed by design.

We want to answer:
- Can memory-latency-bound code cause as much (or more) degradation than
  compute-bound code at the same iteration count?
- How does the TC397 data cache behave under pathological pointer-chase
  patterns with a volatile qualifier?
- Does the analysis model correctly identify a latency-bound workload
  as distinct from a compute-bound one in its commentary?
- Does the analysis tooling handle function symbols that manipulate struct
  types (non-scalar arguments) correctly?

## Target PR / Code Delta
- **Branch:** `test/pointer-chase-labyrinth_2026-02-27_10-15`
- **Change:** Add `perf_chase.c` and `perf_chase.h` containing a volatile
  linked list implementation with 7 functions. All are `noinline` and
  called from `core0_main`'s `while(1)` via an orchestrator.
  - `chase_init_list` — fill a 512-node volatile pool with a pseudo-random
    traversal order (each node's `next` pointer points to a non-adjacent
    node). Called ONCE before `while(1)` to set up the structure.
  - `chase_traverse_forward` — walk all 512 nodes following `volatile next`
    pointers. Accumulates a checksum. Each step is a cache miss.
  - `chase_traverse_reverse` — walk in reverse order (separate `prev`
    pointer chain). Same miss pattern, different direction.
  - `chase_random_write` — write a value to every 7th node in traversal
    order. Forces cache-line dirty + evict on each write.
  - `chase_volatile_sum` — sum the `value` field of all 512 nodes via
    pointer chase. Cannot be reordered or cached by the compiler.
  - `chase_checksum_validate` — re-traverse and compare checksum against a
    stored reference. Returns 0 on match, non-zero on corruption (which
    should never happen — this is a correctness guard).
  - `chase_run_all` — orchestrator. Calls `chase_traverse_forward`,
    `chase_traverse_reverse`, `chase_random_write`, `chase_volatile_sum`,
    and `chase_checksum_validate` every iteration.
- **Note:** `chase_init_list` is called once before `while(1)`, NOT by
  `chase_run_all`. It still appears as a global symbol.
- **Scope:** 1 new `.c`, 1 new `.h`, 1 modified caller.

## Metrics Under Measurement
- [x] Response Time (primary: targeting >100% increase — latency-bound)
- [x] Throughput (model: single PR, baseline expected)
- [x] Power Usage (on-target: cache-miss traffic drives DRAM bus power up)
- [x] Degradation Over Time (model: N runs, single PR)
- [x] Binary Size (.text grows from 7 new functions, likely small per-function)
- [x] Runtime Performance (on-target: every main-loop iteration pays full
  latency cost for 512 pointer hops × 4 traversals)

## Preconditions
- Baseline on `main` HEAD.
- Clean build per Directive #3.
- Same hardware per Directive #5.
- Baseline response time recorded.

## Test Procedure
1. Record baseline on `main` HEAD (`Blinky_LED.elf`).
2. Create branch `test/pointer-chase-labyrinth_2026-02-27_10-15`.
3. Add `perf_chase.c` and `perf_chase.h`.
4. Modify `Cpu0_Main.c`: call `chase_init_list()` before `while(1)`,
   call `chase_run_all()` inside `while(1)`.
5. Build and verify no errors.
6. Commit, push, move test file to `Running/`, open PR.
7. GitHub Actions: clean build, record section sizes.
8. GitHub Actions: flash, on-target timing and power measurements.
9. Record model metrics.
10. Repeat for N runs.
11. Compare against baseline.

## Input / Workload Description
- **Delta size:** Small file count (2 new files + 1 modified).
- **Workload per iteration:** 512 nodes × 4 traversals = 2048 pointer
  hops. Each hop reads a `volatile` pointer from a non-adjacent node —
  guaranteed cache miss on TC397's 32-byte cache lines with a 512-node
  pool sized to overflow L1/L2.
- **Node structure size:** 16 bytes per node (value: 4B, next: 4B,
  prev: 4B, pad: 4B). 512 nodes = 8 KB total pool — chosen to fit above
  TC397's L1 data cache but within accessible DSPR.
- **Nature:** Additive — new files, new calls.

## Expected Results
- **Response time:** >100% increase. 2048 cache misses per iteration at
  TC397 cache-miss penalty (~10–20 cycles each) = 20,480–40,960 wasted
  cycles per iteration. At 300 MHz, that is 68–136 µs added per iteration
  on top of the ~6 µs baseline — a 1,000%+ degradation is plausible if
  pointer-chase is cold-cache every iteration.
- **Binary .text size:** Moderate increase — 7 small-to-medium functions.
  Estimated +1–2 KB.
- **Binary .bss:** Increase from 512-node pool (8 KB). This is a notable
  SRAM cost — document for capacity tracking.
- **Power usage:** Bus power up noticeably. Cache misses → DRAM accesses
  → higher bus utilization. Expected 10–30% increase.
- **Model response time:** Normal — small delta. Similar to PERF-001s.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 7 | All noinline, all extern |
| New local/static symbols | 0 | No static functions |
| Inlined-away functions | 0 | noinline on all |
| Modified existing functions | 1 | `core0_main` — pre-loop init + in-loop run |
| Total function count delta | +7 | Baseline ~171 → expected ~178 |

**New function symbols (all `__attribute__((noinline))`, all GLOBAL):**
- `chase_init_list` — builds the volatile node pool (called once, before loop)
- `chase_traverse_forward` — forward pointer chain walk, 512 hops
- `chase_traverse_reverse` — reverse pointer chain walk, 512 hops
- `chase_random_write` — write to every 7th node in traversal order
- `chase_volatile_sum` — accumulate all node values via pointer chase
- `chase_checksum_validate` — re-traverse and validate checksum
- `chase_run_all` — orchestrator (all except init), called per iteration

**Static data:** `chase_nodes[512]` — 8 KB volatile node pool in `.bss`.
Not a function, so LOCAL/GLOBAL function count unaffected.

## Source-to-Binary Function Correlation

```
Source                         Compilation                ELF Binary
─────                          ───────────                ──────────

perf_chase.c                    GCC TriCore -O2
┌─────────────────────────┐    ┌──────────────┐    ┌──────────────────────────────┐
│ chase_init_list()        │───▶│  noinline    │───▶│ chase_init_list         [GLB]│
│ chase_traverse_forward() │───▶│  all extern  │───▶│ chase_traverse_forward  [GLB]│
│ chase_traverse_reverse() │───▶│  discrete    │───▶│ chase_traverse_reverse  [GLB]│
│ chase_random_write()     │───▶│  symbols.    │───▶│ chase_random_write      [GLB]│
│ chase_volatile_sum()     │───▶│  volatile    │───▶│ chase_volatile_sum      [GLB]│
│ chase_checksum_validate()│───▶│  blocks opt. │───▶│ chase_checksum_validate [GLB]│
│ chase_run_all()          │───▶│              │───▶│ chase_run_all           [GLB]│
└─────────────────────────┘    └──────────────┘    │                              │
                                                    │ chase_nodes[512] → .bss      │
Cpu0_Main.c                                        │ (8 KB volatile pool)         │
┌─────────────────────────┐                        │                              │
│ core0_main()            │                        │ core0_main            [GLB]  │
│   chase_init_list() ◄───┼── ONCE (before loop)  │   CALL chase_init_list       │
│   while(1) {            │                        │   CALL chase_run_all         │
│     chase_run_all() ◄───┼── EVERY ITERATION      │   CALL blinkLED              │
│     blinkLED()          │                        └──────────────────────────────┘
│   }                     │
└─────────────────────────┘       7 new GLOBAL symbols
                                  0 inlined / invisible
                                  1 modified (core0_main)
                                  8 KB added to .bss
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Primary:** Response time must be >100% above baseline.
- **Correctness:** `chase_checksum_validate` must return 0 every iteration.
  Non-zero return = memory corruption = hard fail.
- **Build:** Must compile and link cleanly.
- **SRAM headroom:** 8 KB added to `.bss`. Confirm DSPR budget allows it.
- **Model:** Must process without timeout or crash.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test targets memory-latency degradation specifically. The comparison
to PERF-006 (compute-bound) is intentional — if PERF-007 achieves similar
or higher degradation with far fewer arithmetic operations, that tells us
TC397 is latency-sensitive in its cache hierarchy under pointer-chase
workloads. If PERF-006 vastly outperforms PERF-007 in degradation terms,
the TC397's cache is more effective than expected at handling structured
(even if non-sequential) pointer patterns.

The checksum validator also serves as a built-in correctness test: if the
pointer chase corrupts its own data structure, we catch it immediately.
