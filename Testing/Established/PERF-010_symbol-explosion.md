# Test Template

## Test ID
PERF-010

## Test Name
Symbol Explosion — Analysis Tool Stress via Mixed Symbol Overload

## Objective
Primary goal: stress the analysis tools (generate_report.py, the bot parser,
the scoring logic) by creating a PR with the most complex and mixed symbol
table in the suite. Secondary goal: verify that 100%+ response time
degradation can be achieved even when individual functions are small, simply
by having many of them run per iteration.

This test targets parser edge cases specifically:
- 21 new GLOBAL symbols (highest count for a single binary in the suite)
- 5 new LOCAL symbols (static functions — tests LOCAL vs GLOBAL parsing)
- 8 inlined-away symbols (invisible in binary — tests inference from source)
- Total ELF symbol delta of 26 (21 global + 5 local)

We want to answer:
- Does the analysis tool correctly count and categorize GLOBAL, LOCAL,
  and inlined-away functions when all three are present in the same PR?
- Does the bot's scoring logic handle 100%+ response time deltas without
  arithmetic overflow, NaN, or rendering artifacts?
- Can generate_report.py produce valid output when function counts are
  more than 3x any previous test?
- Does 100%+ response time degradation occur from many small functions
  rather than a few heavy ones?

## Target PR / Code Delta
- **Branch:** `test/symbol-explosion_2026-02-27_11-00`
- **Change:** Add `perf_explode.c` and `perf_explode_helpers.h` containing
  the following symbol categories, all called from `core0_main`'s `while(1)`
  via `explode_run_all()`:

  **GLOBAL functions (21, all `__attribute__((noinline))`):**
  Arithmetic workers (5):
  - `explode_poly_eval` — evaluate a degree-7 polynomial (8 coefficients)
    over 64 input values using Horner's method. Purely arithmetic.
  - `explode_interp_linear` — piecewise linear interpolation over 128
    sample points. Array lookup + multiply-add per sample.
  - `explode_fixed_point_mul` — 64-element fixed-point multiply with
    Q15 rounding (shift + round per element).
  - `explode_running_stats` — compute running mean and variance over a
    128-element window (one pass, Welford's online algorithm).
  - `explode_threshold_count` — count elements in a 256-element array
    that fall within a runtime threshold band. Simple loop, many branches.

  Hash/CRC workers (4):
  - `explode_fnv1a_32` — FNV-1a 32-bit hash over 128 bytes.
  - `explode_djb2_hash` — DJB2 hash over 64-byte string.
  - `explode_crc8_byte` — byte-by-byte CRC-8 over 256 bytes.
  - `explode_checksum16` — 16-bit additive checksum over 512 bytes with
    carry folding.

  Memory workers (4):
  - `explode_memset32` — fill 256-element `uint32_t` buffer with pattern.
  - `explode_memcopy32` — copy 256 elements from src to dst arrays.
  - `explode_memeq32` — compare 256 elements, return first mismatch index.
  - `explode_memreverse32` — in-place reverse a 256-element array.

  Bit manipulation workers (4):
  - `explode_popcount_array` — popcount 128 uint32_t values, sum results.
  - `explode_parity_array` — parity of each byte in 256 bytes, XOR all.
  - `explode_bitrev32` — software bit-reverse of 64 uint32_t values.
  - `explode_clz_sum` — count leading zeros for 128 values, sum counts.

  Sorting/searching workers (3):
  - `explode_insertion_sort` — sort a 64-element volatile array.
  - `explode_binary_search` — binary search in a 256-element sorted array
    for 16 different targets sequentially.
  - `explode_merge_pass` — single merge pass of a 128-element merge sort.

  Orchestrator (1):
  - `explode_run_all` — calls all 20 workers above, every iteration.

  **LOCAL functions (5, all `static __attribute__((noinline))`):**
  Helper internals visible as LOCAL symbols in nm output but not linkable
  externally:
  - `explode_horner_step` — static noinline helper for `explode_poly_eval`
  - `explode_welford_update` — static noinline helper for `explode_running_stats`
  - `explode_merge_combine` — static noinline helper for `explode_merge_pass`
  - `explode_lut_lookup` — static noinline helper for `explode_interp_linear`
  - `explode_carry_fold` — static noinline helper for `explode_checksum16`

  **Inlined-away functions (8, `static inline __attribute__((always_inline))`):**
  Micro-utilities inlined at the single call site each has — zero symbols
  emitted in the binary, but visible in source:
  - `explode_sat_add_u32` — saturating add helper
  - `explode_swap_u32` — inline swap via XOR
  - `explode_min_u32`, `explode_max_u32` — min/max helpers
  - `explode_rotate_left`, `explode_rotate_right` — bit rotate helpers
  - `explode_abs_diff` — absolute difference
  - `explode_round_q15` — Q15 rounding helper

- **Scope:** 1 new `.c` + 1 new `.h` + 1 modified caller.

## Metrics Under Measurement
- [x] Response Time (secondary: targeting >100% from many small functions)
- [x] Throughput (model: medium delta — 3 files, moderate line count)
- [x] Power Usage (on-target: 20 workers per iteration, all paths active)
- [x] Degradation Over Time (model: N runs)
- [x] Binary Size (.text grows substantially from 26 non-inlined symbols)
- [x] Runtime Performance (on-target: aggregate of 20 small workers)
- [x] Analysis Tool Correctness (PRIMARY: parsing mixed GLOBAL/LOCAL/inlined)

## Preconditions
- Baseline on `main` HEAD.
- Clean build per Directive #3.
- Same hardware per Directive #5.
- generate_report.py and bot parser at current version (this test validates
  them — run it after any parser changes to catch regressions).

## Test Procedure
1. Record baseline on `main` HEAD (`Blinky_LED.elf`).
2. Create branch `test/symbol-explosion_2026-02-27_11-00`.
3. Add `perf_explode.c` and `perf_explode_helpers.h`.
4. Modify `Cpu0_Main.c`: add `explode_run_all()` inside `while(1)`.
5. Build and verify no errors.
6. Confirm with `nm` or `objdump`: 21 GLOBAL new symbols, 5 LOCAL new
   symbols, 8 functions present in source but absent from ELF.
7. Commit, push, move test file to `Running/`, open PR.
8. GitHub Actions: clean build, record section sizes.
9. GitHub Actions: flash, on-target timing and power.
10. Run generate_report.py — verify no parse errors, crashes, or NaN values
    in output even with 26 symbols and 100%+ delta percentages.
11. Repeat for N runs.
12. Compare all metrics against baseline.

## Input / Workload Description
- **Delta file count:** 3 (2 new + 1 modified) — small.
- **Symbol complexity:** Mixed — 21 GLOBAL, 5 LOCAL, 8 inlined-away.
  This combination has not appeared in any prior test. Parser stress case.
- **Per-iteration workload:** 20 workers × small-to-medium loops.
  Per-worker cost is individually modest but total across all 20 is
  designed to exceed 100% degradation.
- **Nature:** Additive — new files, new calls.

## Expected Results
- **Response time:** >100% increase expected. 20 workers × avg 50–200 µs
  each (rough estimate for smallest workers) = 1–4 ms added per iteration.
  Given 6 µs baseline, even 10% efficiency would exceed 100% degradation.
- **Analysis tool output:** CRITICAL expected results for this test:
  - `expected_new_global: 21` — parser must count exactly 21 GLOBAL symbols
  - `expected_local_static: 5` — parser must distinguish 5 LOCAL symbols
  - `expected_inlined_away: 8` — parser must infer 8 functions that exist in
    source but are absent from the ELF symbol table
  - Any count mismatch between bot comment and expected values in
    `report_config.json` is flagged as a tool accuracy failure.
- **Binary .text size:** Large increase — 26 non-inlined symbols.
  Estimated +4–8 KB.
- **Binary .data/.bss:** Minimal — most buffers are local (stack).
- **Power usage:** Elevated from 20 workers. Expected 30–70% increase.
- **Model response time:** Moderate — medium-size delta (2 new files).

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 21 | 20 workers + 1 orchestrator, all noinline extern |
| New local/static symbols | 5 | `static noinline` helpers, LOCAL in ELF |
| Inlined-away functions | 8 | `static inline always_inline`, invisible in ELF |
| Modified existing functions | 1 | `core0_main` — adds `explode_run_all()` in while(1) |
| Total function count delta | +26 | 21 GLOBAL + 5 LOCAL (8 inlined invisible) |

**New GLOBAL symbols (all `__attribute__((noinline))`, all extern):**
Arithmetic: `explode_poly_eval`, `explode_interp_linear`,
`explode_fixed_point_mul`, `explode_running_stats`, `explode_threshold_count`

Hash/CRC: `explode_fnv1a_32`, `explode_djb2_hash`, `explode_crc8_byte`,
`explode_checksum16`

Memory: `explode_memset32`, `explode_memcopy32`, `explode_memeq32`,
`explode_memreverse32`

Bit manipulation: `explode_popcount_array`, `explode_parity_array`,
`explode_bitrev32`, `explode_clz_sum`

Sort/search: `explode_insertion_sort`, `explode_binary_search`,
`explode_merge_pass`

Orchestrator: `explode_run_all`

**New LOCAL symbols (all `static __attribute__((noinline))`):**
`explode_horner_step`, `explode_welford_update`, `explode_merge_combine`,
`explode_lut_lookup`, `explode_carry_fold`

**Inlined-away (all `static inline __attribute__((always_inline))`):**
`explode_sat_add_u32`, `explode_swap_u32`, `explode_min_u32`,
`explode_max_u32`, `explode_rotate_left`, `explode_rotate_right`,
`explode_abs_diff`, `explode_round_q15`

## Source-to-Binary Function Correlation

```
Source                              Compilation              ELF Binary
─────                               ───────────              ──────────

perf_explode_helpers.h
┌──────────────────────────────┐   ┌────────────────┐
│ static inline always_inline: │   │ always_inline: │
│  explode_sat_add_u32()       │──▶│  expanded at   │   (no symbols emitted)
│  explode_swap_u32()          │──▶│  each call     │   8 functions inlined away
│  explode_min/max_u32()       │──▶│  site. Zero    │
│  explode_rotate_left/right() │──▶│  ELF symbols   │
│  explode_abs_diff()          │──▶│  emitted.      │
│  explode_round_q15()         │──▶│                │
└──────────────────────────────┘   └────────────────┘

perf_explode.c                       GCC TriCore -O2
┌──────────────────────────────┐   ┌────────────────┐    ┌──────────────────────────────┐
│ static noinline:             │   │ static noinline│    │                              │
│  explode_horner_step()       │──▶│  stays in TU   │───▶│ explode_horner_step    [LOC] │
│  explode_welford_update()    │──▶│  as LOCAL sym  │───▶│ explode_welford_update [LOC] │
│  explode_merge_combine()     │──▶│                │───▶│ explode_merge_combine  [LOC] │
│  explode_lut_lookup()        │──▶│                │───▶│ explode_lut_lookup     [LOC] │
│  explode_carry_fold()        │──▶│                │───▶│ explode_carry_fold     [LOC] │
│                              │   └────────────────┘    │                              │
│ extern noinline (×21):       │   ┌────────────────┐    │ explode_poly_eval      [GLB] │
│  explode_poly_eval()         │──▶│  extern noinl. │───▶│ explode_interp_linear  [GLB] │
│  explode_interp_linear()     │──▶│  all GLOBAL    │───▶│ explode_fixed_point_mul[GLB] │
│  ... (18 more workers) ...   │──▶│  symbols.      │───▶│ ... (18 more)          [GLB] │
│  explode_run_all()           │──▶│                │───▶│ explode_run_all        [GLB] │
└──────────────────────────────┘   └────────────────┘    │                              │
                                                          │ core0_main             [GLB] │
Cpu0_Main.c                                              │   CALL explode_run_all       │
┌──────────────────────────────┐                         │   CALL blinkLED              │
│ core0_main()                 │──── CALL ──────────────▶└──────────────────────────────┘
│   while(1) {                 │
│     explode_run_all() ◄──────┼── EVERY ITERATION
│     blinkLED()               │       21 new GLOBAL symbols
│   }                          │        5 new LOCAL symbols
└──────────────────────────────┘        8 inlined-away (invisible)
                                         1 modified (core0_main)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Analysis tool (PRIMARY PASS/FAIL for this test):**
  - generate_report.py produces output without errors, crashes, or
    NaN/Inf values in any numeric field.
  - Reported GLOBAL count = 21 ± 0.
  - Reported LOCAL count = 5 ± 0.
  - Reported inlined-away count = 8 ± 0.
  - Response time delta renders correctly as a percentage (no overflow
    or formatting artifacts when value exceeds 100%).
- **Runtime (SECONDARY):** Response time >100% above baseline.
- **Build:** Must compile and link cleanly.
- **Model:** Must process without timeout or crash.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test doubles as a regression test for the analysis tooling. Any time
generate_report.py or the bot parser is modified, PERF-010 should be re-run
to confirm the mixed-symbol-category logic still works.

The inlined-away count (8) is interesting: these functions exist in source,
are referenced in source, but produce zero ELF symbols. The analysis tool
must detect them by diffing source-visible function declarations against nm
output. If the tool doesn't do this already, this test will expose the gap.

PERF-010 is the final test in the PERF-006 through PERF-010 degradation
suite. Together, the five tests cover:
- PERF-006: Raw compute degradation (compute-bound)
- PERF-007: Memory latency degradation (latency-bound)
- PERF-008: Branch prediction degradation (pipeline-bound)
- PERF-009: Multi-core contention degradation (bus-bound)
- PERF-010: Symbol complexity + aggregate small-function degradation
             (analysis-tool-bound + many-function overhead)
