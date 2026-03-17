# Test Template

## Test ID
PERF-005s

## Test Name
Add Inline Functions — Small Scale

## Objective
The small-scale counterpart to PERF-005. Add inline qualifiers to a
handful of functions in a single file. This gives us the minimum data
point for inline addition impact — both on-target and for model processing.

We want to answer:
- Is the effect of inlining a few functions even detectable?
- How does the model handle a small additive-qualifier delta?
- What's the binary size cost of inlining just a few functions?

## Target PR / Code Delta
- **Branch:** `test/add-inline-small_<date_time>`
- **Change:** Add `static inline` to 3–5 functions in a single file.
  Pick functions that are called from multiple places to maximize the
  inlining effect even at small scale.
- **Scope:** Small — 1 file, 3–5 qualifier additions.

## Metrics Under Measurement
- [x] Response Time (model: minimal delta)
- [x] Throughput (model: baseline)
- [x] Power Usage (on-target: marginal at this scale)
- [x] Binary Size (small growth from inlined expansions)
- [x] Runtime Performance (on-target: slight improvement possible)

## Preconditions
- Baseline on `main` HEAD.
- Clean build, same hardware per Directives.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/add-inline-small_<date_time>`.
3. Add inline qualifiers to 3–5 functions in one file.
4. Build and verify.
5. Commit, push, move to `Running/`, open PR.
6. GitHub Actions runs full cycle.
7. Repeat for N runs.
8. Compare against baseline.

## Input / Workload Description
- **Delta size:** Small — 1 file, 3–5 qualifier additions.
- **Nature:** Additive — adding qualifiers, no logic change.

## Expected Results
- **Binary .text size:** Increase of 0.5–3%. Each inlined function gets
  duplicated at its call sites. With only 3–5 functions, the growth is
  modest.
- **Binary .data/.bss:** No change.
- **Runtime performance:** Improvement of 0–3%. Call overhead saved for a
  few functions. Detectable only if the functions are in hot paths.
  Likely within noise if they're cold.
- **Power usage:** Within measurement noise. Too few changes to move the
  needle on power.
- **Model response time:** Fast — trivial delta. At or near minimum
  processing time.
- **Model throughput:** At peak.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 0 | All functions are `static inline always_inline`, header-only |
| New local/static symbols | 0 | `always_inline` prevents any symbol emission |
| Inlined-away functions | 3 | All 3 inlined directly into existing `core0_main` |
| Modified existing functions | 1 | `core0_main` — expanded with inlined code |
| Total function count delta | 0 | No new function symbols; code folded into `core0_main` |

**Inlined-away functions (all `static inline __attribute__((always_inline))`):**
- `inl_small_hash` — hash mixing with XOR and multiply
- `inl_small_rotl` — rotate-left helper
- `inl_small_mix` — combines hash + rotl with constants `0xDEADBEEF`, `0xCAFEBABE`

**Inline vs Not-Inline:** ALL 3 are `always_inline`, defined in header only
(`perf_inline_small.h`). No separate `.c` file. Zero new symbols — the most
extreme inline variant. LOCI must detect changes from `core0_main` body growth.

## Source-to-Binary Function Correlation

```
Source                              Compilation                 ELF Binary
─────                               ───────────                 ──────────

perf_inline_small.h (HEADER ONLY)   GCC TriCore -O2
┌─────────────────────────────┐     ┌────────────────┐
│ static inline always_inline │     │  Header-only.  │
│  inl_small_hash()           │────▶│  All expanded  │     NO NEW SYMBOLS
│  inl_small_rotl()           │────▶│  at #include   │─────────────┐
│  inl_small_mix()            │────▶│  site.         │             │
└─────────────────────────────┘     └────────────────┘             ▼
                                                     ┌──────────────────────────┐
Cpu0_Main.c                                         │ core0_main         [GLB] │
┌─────────────────────────────┐                     │   inl_small_mix()        │
│ core0_main()                │                     │   inlined HERE           │
│   while(1) {                │──── INLINED ───────▶│   (no CALL, code pasted) │
│     inl_small_mix(...)      │                     │   CALL blinkLED          │
│     blinkLED()              │                     └──────────────────────────┘
│   }                         │
└─────────────────────────────┘      0 new symbols (all inlined)
                                     3 INLINED (invisible)
                                     1 modified (core0_main body grows)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Build:** Must compile cleanly.
- **Binary size:** Small growth acceptable. More than 5% growth from
  3–5 inlines indicates something unexpected (recursive inlining,
  template expansion).
- **Model:** Must process quickly and correctly.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
Paired with PERF-005 (large). Comparing small vs large inline addition
shows the scaling curve — does inlining benefit diminish or turn negative
as you apply it to more functions?
