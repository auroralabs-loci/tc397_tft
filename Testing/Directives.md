# Testing Directives — Constant Restrictions

> These directives apply to **all** tests. They are non-negotiable constraints
> that must be satisfied before any test is considered valid.

---

## 1. Binary-Level Testing Only

We don't test source code — we test what actually runs on the chip. The project
gets compiled into a binary (`.elf` or `.hex` file), and that binary is what we
flash and measure. If the code doesn't compile, there is nothing to test.
Looking at source code and guessing how it will perform doesn't count.
The binary is the truth.

## 2. Consistent Build Configuration

Every test in the same test cycle must be built the exact same way — same
compiler (tricore-gcc), same target flag (`-mtc162`), same everything. We use
the `CMakeLists.txt` and toolchain file that are already checked into the repo.
Nobody tweaks compiler flags on the side unless that tweak is literally what
we're testing. This way, when we compare results between two PRs, the only
difference is the code change — not some flag someone forgot they added.
We also keep `CMAKE_EXPORT_COMPILE_COMMANDS` turned on so anyone can go back
and verify exactly how a build was produced.

## 3. Clean Build Per Test

Before every test run, we wipe the entire build folder and rebuild from
scratch (`just clean`, then `configure`, then `build`). No leftovers from a
previous build. This matters because incremental builds can leave stale
object files around — old compiled pieces that didn't get recompiled even
though they should have. That can silently corrupt your measurements. A clean
build guarantees that what you built matches what's in the source tree right now.

## 4. Single Variable Per Test

Change one thing at a time. If you're testing the effect of a specific PR,
then everything else — the toolchain, the flags, the hardware, the
environment — stays exactly the same. If you change two things at once and
the numbers move, you can't tell which change caused it. This is basic
scientific method: isolate the variable you're studying.

## 5. Hardware Environment Lock

The physical board, the debugger, the flash cable, the power supply — all of
it stays the same for every run of a given test. If you swap to a different
TC397 board or use a different debugger probe, you've introduced unknowns.
Different boards can have slightly different clock behavior or power
characteristics. Write down exactly which board revision, clock config, and
supply voltage you're using. If any of that matters to the measurement
(especially power and timing), it needs to be documented and held constant.

## 6. Baseline Required

You can't say "this PR made things faster" or "this PR costs more power"
without knowing what "normal" looks like. Before testing any code change,
we first measure the system on a known-good reference point — typically the
current HEAD of `main`. That measurement is the baseline. Every test result
is compared against it. Without a baseline, the numbers are just numbers —
they don't tell you whether things got better or worse.

## 7. Reproducibility Minimum

Run it more than once. A single measurement can be thrown off by any number
of things — a temperature fluctuation, a timing race, an interrupt firing at
a weird moment. We define a minimum number of runs for each test (N), run it
that many times, and report the average along with how much the results
varied (standard deviation). If the spread is too wide, the test might not
be trustworthy and needs investigation. One run proves nothing.

## 8. Binary Size Tracking

For every test run, we record how big the compiled binary is — specifically
the sizes of the `.text` section (your actual code), `.data` section
(initialized variables), and `.bss` section (zero-initialized variables).
This is a sanity check. If a small code change causes the binary to grow by
an unexpected amount, something might be wrong — maybe the compiler pulled
in an extra library, or an optimization got disabled. Size changes are often
the first sign of an unintended side effect.

## 9. No Debug Payloads in Measured Builds

When we measure performance, the binary must be clean — no leftover
`printf` calls, no debug logging, no extra instrumentation code that
wouldn't be there in a real build. That stuff takes up CPU time, memory,
and power, and it will skew your results. The `-g` flag is fine to keep
because it only adds debug symbols for the debugger to use — it doesn't
change what the CPU actually executes. But anything that adds runtime
behavior to the binary has to go, unless the whole point of the test is
to measure how much that instrumentation costs.

## 10. Results Are Immutable

Once you write down a test result, you don't go back and change it. If you
made a mistake or want to re-run the test, you create a new entry with the
new results. The old entry stays as-is. This keeps an honest history. You
can always see what happened, when it happened, and whether a re-run gave
different numbers. Editing old results destroys that trail and makes it
impossible to trust the data later.

## 11. Test Workflow — Branch, PR, Run

Every test follows a strict lifecycle tied to git:

**Step 1 — Branch creation.**
When we define a new test, it gets its own branch. The branch name follows
the format: `test/<test-name>_<YYYY-MM-DD_HH-MM>`. For example,
`test/small-delta-throughput_2026-02-25_14-30`. The name tells you what the
test is and exactly when it was created. No ambiguity, no collisions.

**Step 2 — Test preparation on the branch.**
All the work for that test — the code delta, the filled-out test template,
any config changes — lives on that branch. The test file sits in
`Testing/Established/` on that branch. Nothing gets merged yet.

**Step 3 — PR creation moves the test to Running.**
When the branch is ready and a PR is opened against `main`, the test
officially enters the **Running** state. The test markdown file moves from
`Testing/Established/` into `Testing/Running/`. This is critical because
`Testing/Running/` is the folder that **GitHub Actions reads from** to know
which tests to execute and validate. If a test file is in `Running/`, CI
picks it up. If it's not there, CI ignores it. The PR triggers the GitHub
Actions pipeline, which:
1. Reads the test file from `Testing/Running/` to know what to measure.
2. Performs a clean build of the binary.
3. Runs the on-target and model measurements defined in the test.
4. Validates results against the expected results and pass/fail criteria
   written in the test markdown.

We do not run tests manually. GitHub Actions is the single source of
execution. The test markdown in `Testing/Running/` is both the instruction
sheet and the validation contract.

**Step 4 — After the PR completes.**
Once GitHub Actions finishes and results are collected, the PR is merged
(or closed). The test file moves out of `Running/` and back into
`Established/` with its `Actual Results` and `Verdict` sections filled in,
and `State.md` gets updated.

**Folder meaning at a glance:**
- `Testing/Established/` — tests that are defined and finalized (either
  waiting to run or already completed with results)
- `Testing/Running/` — active tests that GitHub Actions reads from to
  execute and validate. An open PR = test file in this folder.

**Why this matters:**
The `Running/` folder is the handshake between us and CI. Putting a test
file there tells GitHub Actions "run this and check the results." Taking
it out tells CI "this one is done." The branch name gives traceability,
the PR shows the code delta, and the folder location drives automation.

---

## Suggestions — Review and Confirm

> The following are proposed additions. Confirm, modify, or reject each.

### A. Flash Verification

After we flash the binary onto the board, we read it back (or check a
checksum) to make sure what's on the chip is actually what we intended to
put there. Flash can fail silently — partial writes, corrupted transfers,
wrong file flashed by accident. If we don't verify, we might spend time
measuring the wrong binary and not even know it.

### B. Warm-Up / Settle Period

Right after you flash or reset the board, the system goes through startup —
clocks stabilize, peripherals initialize, caches fill up. If you start
measuring immediately, you're capturing that transient behavior, not the
steady-state performance you actually care about. A warm-up period lets the
system settle into its normal operating state before we start recording data.
How long that period is depends on the test, but it needs to be defined and
consistent.

### C. Power Measurement Calibration

If we're measuring how much power the chip draws, the measurement tool itself
needs to be trustworthy. A shunt resistor, a current sensor, whatever we're
using — it should be calibrated against a known reference load before each
test session. Without calibration, a 5% drift in your sensor looks like a 5%
change in power consumption, and you'd chase a ghost.

### D. Commit Pinning

Record the exact git commit hash for both the code under test and the
baseline. Not the branch name — the hash. Branch names move. Someone pushes
a new commit to `main` and suddenly your "baseline" refers to different code
than when you ran the test. The commit hash is permanent — it always points
to the exact same code, forever. This is how you make results traceable.

### E. Timeout / Watchdog Policy

Set a maximum time limit for each test. If the board hangs, enters an
infinite loop, or just takes way longer than it should, we don't wait
forever — we cut it off and record the run as a failure with a timeout
reason. Without a timeout, a hung test blocks everything behind it, and
worse, someone might silently discard the failed run instead of recording
it. A timeout makes hangs visible and keeps the test pipeline moving.

### F. Artifact Retention

Keep the actual binary files (`.elf`, `.hex`) and the build logs from every
measured run. Don't just record the numbers and throw away the binary. If a
result looks suspicious weeks later, you want to be able to go back, reflash
that exact binary, and re-measure. If you only kept the numbers, you'd have
to rebuild from source and hope the build is bit-identical — which it might
not be. Keeping the artifacts makes everything auditable and reproducible.
