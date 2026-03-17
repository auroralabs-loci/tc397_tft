# Test Template

## Test ID
PERF-002

## Test Name
Assembly Block Delta

## Objective
Test how the performance model handles code deltas that contain inline
assembly blocks. Assembly is opaque to most analysis tools — the model
can't reason about it the same way it reasons about C. This test
introduces hand-written TriCore assembly into critical paths and measures
whether the model can process the delta correctly and how the binary
actually behaves on-target.

We want to answer:
- Can the model process PRs that contain inline assembly without errors
  or degraded analysis quality?
- How does response time change when the delta includes asm blocks vs
  pure C?
- What is the on-target performance difference when key routines are
  replaced with hand-written assembly?

## Target PR / Code Delta
- **Branch:** `test/assembly-blocks_<date_time>`
- **Change:** Replace selected C function bodies with equivalent inline
  assembly (`__asm volatile (...)`) in performance-sensitive paths.
  Candidates:
  - Core initialization sequences (Cpu0_Main through Cpu5_Main)
  - Inner loops in Blinky_LED or Multicore timing routines
  - Any tight loops or register-manipulation code in Libraries/
- **Scope:** Medium — focused changes but the content (raw asm) is what
  makes this interesting, not the file count.

## Metrics Under Measurement
- [x] Response Time (model processing time for an asm-heavy delta)
- [x] Throughput (deltas/sec — does asm content slow the pipeline?)
- [x] Power Usage (on-target: hand-tuned asm may be more or less efficient)
- [x] Degradation Over Time (repeated asm deltas — does model quality drop?)
- [x] Binary Size (.text changes when compiler-generated code is replaced)
- [x] Runtime Performance (on-target timing: asm vs compiler-generated)

## Preconditions
- Baseline measurements on `main` HEAD (pure C, no added asm).
- Assembly must be valid TriCore TC1.6.2 (`-mtc162`) instruction set.
- Clean build per Directive #3.
- Same hardware per Directive #5.

## Test Procedure
1. Record baseline on `main` HEAD.
2. Create branch `test/assembly-blocks_<date_time>`.
3. Identify target functions and rewrite their bodies in inline asm.
4. Verify the build compiles and links cleanly.
5. Commit, push, move test file to `Running/`, open PR.
6. GitHub Actions: clean build, record section sizes.
7. GitHub Actions: flash, run on-target timing and power measurements.
8. Record model response time and throughput for processing the delta.
9. Repeat for N runs.
10. Compare all metrics against baseline.

## Input / Workload Description
- **Delta size:** Medium file count, but content is dense — raw assembly
  is harder to parse per line than C.
- **Nature:** Replacement — C function bodies swapped for asm equivalents.
  External interfaces (function signatures) stay the same.
- **Assembly style:** GCC extended inline asm with input/output operands
  and clobber lists.

## Expected Results
- **Model response time:** Likely higher per delta than equivalent C
  changes — assembly is less structured and harder to analyze.
- **Binary .text size:** Could go either way — hand-written asm may be
  tighter or may miss optimizations the compiler would have applied.
- **Runtime performance:** If the asm is well-written, equal or better
  than compiler output. If naive, could be worse.
- **Power:** Tighter asm loops may reduce power. More memory accesses
  from poor register allocation would increase it.

## Expected Function Counts

| Category | Count | Details |
|----------|-------|---------|
| New global symbols | 7 | All extern functions containing TriCore inline asm |
| New local/static symbols | 0 | No static functions |
| Inlined-away functions | 0 | No inline qualifiers; `asm volatile` blocks prevent optimization |
| Modified existing functions | 1 | `core0_main` — adds call to `asm_run_all()` in main loop |
| Total function count delta | +7 | Baseline ~171 → expected ~178 |

**New function symbols (all extern, all contain `__asm__ volatile`, all GLOBAL):**
- `asm_saturated_add_array` — TriCore `adds.h` packed saturated addition
- `asm_crc32_byte` — CRC via `xor`, `and`, `rsub`, `sh` instructions
- `asm_memory_fill` — memory fill via `st.w`
- `asm_count_leading_zeros` — TriCore `cls` instruction
- `asm_bit_reverse` — `extr.u` + `insert` bit manipulation
- `asm_packed_abs` — `abs.h` packed absolute value
- `asm_run_all` — orchestrator, calls all above

**Inline vs Not-Inline:** None are inline. The `asm volatile` blocks guarantee
each function appears as a discrete global symbol with assembly embedded in .text.

## Source-to-Binary Function Correlation

```
Source                       Compilation                  ELF Binary
─────                        ───────────                  ──────────

perf_asm.c                    GCC TriCore -O2
┌──────────────────────┐     ┌───────────────┐    ┌─────────────────────────────┐
│ asm_saturated_add()  │────▶│  asm volatile │───▶│ asm_saturated_add     [GLB] │
│ asm_crc32_byte()     │────▶│  blocks are   │───▶│ asm_crc32_byte        [GLB] │
│ asm_memory_fill()    │────▶│  emitted       │───▶│ asm_memory_fill       [GLB] │
│ asm_count_lead_z()   │────▶│  verbatim.    │───▶│ asm_count_leading_z   [GLB] │
│ asm_bit_reverse()    │────▶│  Compiler      │───▶│ asm_bit_reverse       [GLB] │
│ asm_packed_abs()     │────▶│  cannot        │───▶│ asm_packed_abs        [GLB] │
│ asm_run_all()        │────▶│  optimize asm │───▶│ asm_run_all           [GLB] │
└──────────────────────┘     └───────────────┘    │                             │
                                                   │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─│
Cpu0_Main.c                                       │ core0_main            [GLB] │
┌──────────────────────┐                          │   CALL asm_run_all          │
│ core0_main()         │──── CALL ───────────────▶│   CALL blinkLED             │
│   while(1) {         │                          └─────────────────────────────┘
│     asm_run_all()    │
│     blinkLED()       │         7 new GLOBAL symbols
│   }                  │         0 inlined / invisible
└──────────────────────┘         1 modified (core0_main)
```

## Actual Results
<!-- Filled in after execution -->

## Pass / Fail Criteria
- **Model:** Must process the delta without crash or timeout. Response
  time recorded for benchmarking.
- **Build:** Must compile and link. Any asm syntax error = test blocked,
  not failed.
- **Functional:** Binary must behave identically to baseline (same
  observable output). Assembly rewrites must be functionally equivalent.
- **Performance:** Timing and power recorded and compared. Thresholds TBD.

## Verdict
<!-- PASS | FAIL | INCONCLUSIVE -->

## Notes / Observations
<!-- Anything unexpected, follow-up items, environment anomalies -->
This test specifically targets the model's blind spot. Most analysis tools
treat inline asm as a black box. We want to know if that limitation causes
measurable issues in processing time, accuracy, or throughput. The on-target
measurements are secondary validation — they confirm the asm is correct.
