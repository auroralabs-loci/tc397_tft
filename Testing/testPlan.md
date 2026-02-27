# Test Plan — Performance Processing for Code Deltas

## Scope
Stress testing the performance-processing model against various PR scenarios on the remote.
Key metrics: response times, throughput, power usage, degradation, and related performance indicators.
All tests executed via GitHub Actions. Test files in `Testing/Running/` drive CI execution and validation.

---

## Environments
| Environment | Description | Status |
|-------------|-------------|--------|
| Remote (GitHub Actions) | CI pipeline — builds, flashes, measures | Pending setup |
| Target Hardware | TC397 TFT | Pending confirmation |

---

## Test Matrix

| Test ID | Scenario | Scale | Key Metrics | Priority | Status |
|---------|----------|-------|-------------|----------|--------|
| PERF-001 | Remove inline functions | Large | Binary size, runtime, model response | High | Defined |
| PERF-001s | Remove inline functions | Small | Binary size, runtime, model response | High | Defined |
| PERF-002 | Assembly block deltas | Large | Model handling of asm, runtime | High | Defined |
| PERF-002s | Assembly block delta | Small | Model handling of asm, runtime | Medium | Defined |
| PERF-003 | Massive degradation stress | Large | Degradation curve, recovery, errors | Critical | Defined |
| PERF-003s | Degradation via volume | Small | Throughput ceiling, recovery | High | Defined |
| PERF-004 | Massive improvement | Large | Binary size, runtime, power, model | High | Defined |
| PERF-004s | Small improvement | Small | Detection sensitivity, runtime | Medium | Defined |
| PERF-005 | Add inline functions | Large | Binary size, cache pressure, runtime | High | Defined |
| PERF-005s | Add inline functions | Small | Binary size, runtime | Medium | Defined |
| PERF-006 | Nested loop bloat (per-iteration) | Large | Response time >100%, compute-bound | Critical | Defined |
| PERF-007 | Pointer chase labyrinth | Large | Response time >100%, latency-bound | Critical | Defined |
| PERF-008 | Branch prediction destroyer | Large | Response time >100%, pipeline-bound | Critical | Defined |
| PERF-009 | Six-core in-loop cascade | Large | Per-core >100% degradation, crossbar, power | Critical | Defined |
| PERF-010 | Symbol explosion | Large | >100% degradation + analysis tool stress | Critical | Defined |

---

## Test Pairings

Each scenario in the original suite has a large and small variant. This lets us
separate the effect of the code change itself from the effect of delta size on
model processing.

| Pair | Large | Small | What the comparison reveals |
|------|-------|-------|-----------------------------|
| Inline removal | PERF-001 | PERF-001s | Scaling of inline removal impact |
| Assembly blocks | PERF-002 | PERF-002s | Asm content vs asm volume effect on model |
| Degradation | PERF-003 | PERF-003s | Size-driven vs volume-driven degradation |
| Improvement | PERF-004 | PERF-004s | Optimization detection at different scales |
| Inline addition | PERF-005 | PERF-005s | Inlining benefit curve and diminishing returns |

## 100%+ Degradation Suite (PERF-006 to PERF-010)

Five single-variant tests, each targeting >100% response time degradation via a
different bottleneck mechanism. No small variants — these are designed to be
extreme by definition.

| Test | Mechanism | TC397 Bottleneck | Analysis Tool Target |
|------|-----------|------------------|----------------------|
| PERF-006 | Nested loop bloat | Integer pipeline (compute-bound) | 7 new GLOBAL, standard |
| PERF-007 | Pointer chase labyrinth | D-cache / memory latency | 7 new GLOBAL, 8 KB .bss |
| PERF-008 | Branch prediction destroyer | Branch predictor / pipeline flush | 7 new GLOBAL, data-dependent |
| PERF-009 | Six-core in-loop cascade | SRI crossbar / bus arbitration | 19 new GLOBAL, 6 cores |
| PERF-010 | Symbol explosion | Aggregate small-function overhead | 21 GLOBAL + 5 LOCAL + 8 inlined |

---

## Execution Order (Recommended)

1. **Baseline** — Establish reference measurements on `main` HEAD.
2. **PERF-001s + PERF-005s** — Small inline tests first. Quick, validates the pipeline.
3. **PERF-001 + PERF-005** — Large inline tests. Brackets the inlining range.
4. **PERF-002s then PERF-002** — Assembly tests, small then large.
5. **PERF-004s then PERF-004** — Improvement tests, small then large.
6. **PERF-003s then PERF-003** — Degradation tests (stress the system hardest in the original suite).
7. **PERF-006** — Nested loop bloat. Confirms 100%+ degradation is achievable;
   establishes compute-bound baseline for the extreme suite.
8. **PERF-007** — Pointer chase. Cache-miss comparison to PERF-006.
9. **PERF-008** — Branch destroyer. Pipeline comparison to PERF-006/007.
10. **PERF-009** — Six-core cascade. Multi-core, highest power test; run after
    single-core tests so thermal state is documented.
11. **PERF-010** — Symbol explosion. Run last — it also validates the analysis
    tooling, so prior results must be in place for meaningful comparison.

---

## Schedule
| Phase | Description | Target Date | Owner |
|-------|-------------|-------------|-------|
| Planning | Define scenarios and thresholds | In progress | — |
| Pipeline Setup | GitHub Actions workflow for `Testing/Running/` | TBD | — |
| Execution | Run tests on remote | TBD | — |
| Analysis | Collect and review results | TBD | — |

---

## Open Questions
- Exact pass/fail thresholds for each metric
- Number of repetitions (N) per test
- GitHub Actions workflow configuration for reading `Testing/Running/`
- Baseline commit to pin as reference
