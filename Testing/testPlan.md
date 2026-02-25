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

---

## Test Pairings

Each scenario has a large and small variant. This lets us separate the effect of
the code change itself from the effect of delta size on model processing.

| Pair | Large | Small | What the comparison reveals |
|------|-------|-------|-----------------------------|
| Inline removal | PERF-001 | PERF-001s | Scaling of inline removal impact |
| Assembly blocks | PERF-002 | PERF-002s | Asm content vs asm volume effect on model |
| Degradation | PERF-003 | PERF-003s | Size-driven vs volume-driven degradation |
| Improvement | PERF-004 | PERF-004s | Optimization detection at different scales |
| Inline addition | PERF-005 | PERF-005s | Inlining benefit curve and diminishing returns |

---

## Execution Order (Recommended)

1. **Baseline** — Establish reference measurements on `main` HEAD.
2. **PERF-001s + PERF-005s** — Small inline tests first. Quick, validates the pipeline.
3. **PERF-001 + PERF-005** — Large inline tests. Brackets the inlining range.
4. **PERF-002s then PERF-002** — Assembly tests, small then large.
5. **PERF-004s then PERF-004** — Improvement tests, small then large.
6. **PERF-003s then PERF-003** — Degradation tests last (they stress the system hardest).

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
