# TriCore TC397 Performance Testing Knowledge Base

## Compiled Binary Performance Differences on Infineon AURIX TC397

This document serves as a comprehensive knowledge base for designing test code that creates
measurable performance differences between compiled binaries on the Infineon AURIX TC397
microcontroller (6 TriCore cores, TC1.6.2 ISA, tricore-gcc toolchain).

---

## Table of Contents

1. [Response Times in Embedded CPUs](#1-response-times-in-embedded-cpus)
2. [Throughput in Embedded CPUs](#2-throughput-in-embedded-cpus)
3. [Power Usage in Embedded CPUs](#3-power-usage-in-embedded-cpus)
4. [TriCore TC397 Specifics](#4-tricore-tc397-specifics)
5. [Code Patterns That Create Measurable Binary Differences](#5-code-patterns-that-create-measurable-binary-differences)
6. [Stress Test Patterns](#6-stress-test-patterns)

---

## 1. Response Times in Embedded CPUs

### 1.1 What Determines Instruction-Level Response Time

Response time (latency) in an embedded CPU is the wall-clock time from when an event occurs
(interrupt, trigger, input change) to when the processor produces a result or output. On the
TC397, the primary factors are:

#### 1.1.1 Pipeline Behavior

The TriCore TC1.6.2 has a relatively short pipeline (4-5 stages depending on the instruction
class). Key characteristics:

- **No speculative execution**: TriCore does not speculatively execute past branches. A taken
  branch flushes the pipeline, costing 2-3 cycles.
- **No out-of-order execution**: Instructions execute strictly in program order. A stall on
  one instruction blocks everything behind it.
- **Dual-issue capability**: The TC1.6.2 IP core in TC397 (TC1.6.2P) can issue up to two
  instructions per cycle -- one integer/logic and one load/store -- provided there are no
  data dependencies. This is a form of static superscalar execution.

#### 1.1.2 Branch Prediction (or Lack Thereof)

TriCore TC1.6.2 has **no dynamic branch predictor** (no branch history table, no BTB). Instead
it uses a static prediction scheme:

- **Backward branches** (loops) are predicted **taken** (loop-back assumption).
- **Forward branches** are predicted **not taken**.
- Misprediction penalty: ~2-3 cycles (pipeline flush).

This means that code with unpredictable forward branches will incur consistent misprediction
penalties. This is a significant lever for creating response time differences between binaries.

#### 1.1.3 Memory Wait States and Flash vs RAM Execution

This is the single largest factor affecting response time on TC397:

| Memory Region | Access Latency (CPU cycles) | Notes |
|---|---|---|
| DSPR (local) | 0 wait states (1 cycle) | Data Scratch-Pad RAM, per-core, 240KB (CPU0/1) or 96KB (CPU2-5) |
| PSPR (local) | 0 wait states (1 cycle) | Program Scratch-Pad RAM, per-core, 64KB each |
| DLMU / LMU | 1-3 wait states | Shared RAM, depends on arbitration |
| EMEM | 2-4 wait states | Extended memory (4MB), shared |
| PFlash (cached) | 0 wait states (cache hit) / 4-8+ cycles (miss) | Program flash with cache line fill on miss |
| PFlash (uncached, 0xA0xxxxxx) | 4-8+ wait states per access | No caching, every fetch goes to flash |
| DFlash | 10-20+ wait states | Data flash, very slow |

The TC397 PFlash runs at a lower frequency than the CPU. At 300 MHz CPU clock, PFlash
typically requires **4-6 wait states** for a cache miss. The TC397 has a **program cache
(PCACHE)** of 16KB per core and a **data cache (DCACHE)** of 16KB per core (for CPU0 and
CPU1 only; CPU2-5 have smaller or no data cache).

**Key insight**: Code that fits entirely in PSPR runs at full speed with deterministic timing.
Code that runs from flash and constantly misses the PCACHE can be 3-8x slower.

#### 1.1.4 Cache Behavior

TC397 cache specifics:
- **PCACHE**: 16KB, 2-way set-associative, 256-bit (32-byte) cache lines.
- **DCACHE**: Present on CPU0 and CPU1 only, 16KB, line fill from LMU/flash.
- **Cache invalidation**: Can be done programmatically via `IfxCpu_invalidateProgramCache()`.
- **Segment cacheability**: Segments 0x8xxxxxxx (PFlash) and 0x9xxxxxxx (LMU) are cacheable.
  Segment 0xAxxxxxxx is the non-cached mirror of PFlash.

Cache-hostile code patterns include:
- Large working sets that exceed cache size (thrashing).
- Strided memory access patterns that map to the same cache sets (conflict misses).
- Self-modifying code (forces cache invalidation).

#### 1.1.5 Interrupt Latency

TriCore interrupt latency consists of:
1. **Pipeline drain**: 0-3 cycles (finish current instruction).
2. **Context save (SVLCX)**: The hardware automatically saves the lower context (16 registers)
   to the CSA (Context Save Area). This takes approximately **7-10 cycles** on TC1.6.2.
3. **Vector fetch**: Jump to interrupt vector table entry. 1-2 cycles if in cache.
4. **ISR prologue**: If the ISR saves upper context (STUCX) or does additional setup.

Total minimum interrupt latency: approximately **10-15 cycles** from interrupt assertion to
first ISR instruction executing, assuming the vector table is cached and CSA is in local DSPR.

Factors that increase interrupt latency:
- CSA located in slow memory (LMU/EMEM instead of DSPR).
- Vector table in uncached flash.
- Higher-priority interrupt already being serviced.
- `__disable()` / critical sections blocking interrupt recognition.

#### 1.1.6 Writing C Code for Different Response Time Profiles

To create two binaries with measurably different response times:

```c
#include "IfxCpu.h"
#include "IfxStm.h"

/* ---------- FAST RESPONSE: Data and code in scratch-pad RAM ---------- */

/* Place critical data in DSPR */
static volatile uint32 input_buffer_fast[64]
    __attribute__((section(".data_cpu0")));

/* Place critical function in PSPR */
__attribute__((section(".psram_cpu0")))
void fast_response_handler(void)
{
    /* All data is local DSPR -- 0 wait state access */
    uint32 sum = 0;
    for (int i = 0; i < 64; i++) {
        sum += input_buffer_fast[i];
    }
    /* Direct register write -- minimal latency */
    volatile uint32 *output = (volatile uint32 *)0xF003B000; /* Port register */
    *output = sum;
}

/* ---------- SLOW RESPONSE: Data and code in flash ---------- */

/* Data in flash/LMU (high latency) */
static const volatile uint32 input_buffer_slow[64]
    __attribute__((section(".rodata")));

/* Code stays in PFlash (default) -- subject to cache misses */
void slow_response_handler(void)
{
    /* Deliberately access data through non-cached address */
    volatile uint32 *p = (volatile uint32 *)0xAF000000; /* DFlash -- very slow */
    uint32 sum = 0;
    for (int i = 0; i < 64; i++) {
        sum += p[i]; /* 10-20 wait states per access */
    }
    volatile uint32 *output = (volatile uint32 *)0xF003B000;
    *output = sum;
}
```

### 1.2 Measuring Response Time on TC397

The TC397 provides hardware performance counters accessible via Core Special Function
Registers (CSFRs):

```c
#include "IfxCpu.h"

void measure_response_time(void)
{
    /* Reset and start all performance counters */
    IfxCpu_resetAndStartCounters(IfxCpu_CounterMode_normal);

    /* --- Code under test --- */
    function_to_measure();
    /* --- End code under test --- */

    /* Stop counters and read results */
    IfxCpu_Perf perf = IfxCpu_stopCounters();

    uint32 cycles      = perf.clock.counter;       /* CPU clock cycles */
    uint32 instructions = perf.instruction.counter; /* Instructions retired */
    /* perf.counter1/2/3 can be configured for cache misses, etc. */
}
```

Alternatively, use the STM (System Timer) for wall-clock measurements:

```c
#include "IfxStm.h"

uint32 measure_wall_clock(void)
{
    uint32 start = IfxStm_getLower(&MODULE_STM0);
    function_to_measure();
    uint32 end = IfxStm_getLower(&MODULE_STM0);
    return end - start; /* In STM ticks (typically fSPB, e.g., 100 MHz) */
}
```

---

## 2. Throughput in Embedded CPUs

### 2.1 What Determines Computational Throughput

Throughput is the amount of useful work completed per unit time. On an embedded processor
like TC397, the key determinants are:

#### 2.1.1 Instructions Per Cycle (IPC)

TC1.6.2P can issue up to **2 instructions per cycle** (dual issue):
- One from the **Integer Pipeline (IP)**: ALU, multiply, shift, bit operations.
- One from the **Load/Store Pipeline (LS)**: Memory loads, stores, address arithmetic.

Conditions for dual issue:
- No data dependency between the two instructions.
- One must be IP-class, the other LS-class.
- Both must be available (not stalled).

Practical IPC ranges from 0.3 (heavy cache misses, many stalls) to ~1.8 (well-optimized
DSPR-resident code with good instruction mix).

#### 2.1.2 Memory Bandwidth

TC397 memory bandwidth bottlenecks:

| Interface | Bandwidth | Notes |
|---|---|---|
| DSPR (local) | 1 word/cycle (4 bytes @ 300 MHz = 1.2 GB/s) | Single-port SRAM |
| PSPR (local) | 1 word/cycle | Single-port SRAM |
| SRI bus (crossbar) | Shared among all masters | Up to 4.8 GB/s aggregate, but contention reduces per-master BW |
| PFlash (burst) | 256-bit line fill | Amortized over cache line, ~2-3 GB/s peak |

#### 2.1.3 DMA Contention

The TC397 DMA controller shares the SRI (Shared Resource Interconnect) crossbar with CPU
cores. When DMA channels are actively transferring data:
- CPU accesses to the same memory target (e.g., LMU) will stall due to arbitration.
- The SRI uses priority-based arbitration; DMA can be configured to have high or low priority.
- Heavy DMA traffic to/from LMU while CPUs also access LMU can degrade CPU throughput by
  20-50%.

#### 2.1.4 Bus Arbitration in Multicore Systems

The TC397 SRI crossbar connects 6 CPU cores, the DMA controller, and other bus masters to
various memory targets. Arbitration occurs at each memory target slave port:

- **Round-robin** or **priority-based** arbitration depending on configuration.
- **Worst case**: All 6 cores + DMA simultaneously requesting the same EMEM bank --
  throughput per core drops to roughly 1/7th of single-core throughput.
- **Best case**: Each core works exclusively in its own DSPR -- zero contention.

#### 2.1.5 Code Patterns Affecting Throughput

**Loop Unrolling**:
```c
/* Baseline: Tight loop, 1 operation per iteration */
void sum_baseline(const uint32 *data, uint32 n, uint32 *result)
{
    uint32 sum = 0;
    for (uint32 i = 0; i < n; i++) {
        sum += data[i];
    }
    *result = sum;
}

/* Unrolled: 4 operations per iteration, reduces loop overhead */
void sum_unrolled(const uint32 *data, uint32 n, uint32 *result)
{
    uint32 sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
    uint32 i;
    for (i = 0; i + 3 < n; i += 4) {
        sum0 += data[i];
        sum1 += data[i + 1];
        sum2 += data[i + 2];
        sum3 += data[i + 3];
    }
    /* Handle remainder */
    for (; i < n; i++) {
        sum0 += data[i];
    }
    *result = sum0 + sum1 + sum2 + sum3;
}
```

Using 4 independent accumulators (`sum0`-`sum3`) breaks the data dependency chain, allowing
the pipeline to keep all functional units busy. On TriCore, this can improve throughput by
30-60% for compute-bound loops.

**Data Alignment**:
```c
/* Aligned data -- ensures LD.W can use efficient addressing */
static uint32 aligned_data[256] __attribute__((aligned(8)));

/* Misaligned data -- forces byte-by-byte access or multiple loads */
#pragma pack(1)
typedef struct {
    uint8  pad;
    uint32 values[256]; /* Offset by 1 byte -- misaligned */
} MisalignedStruct;
#pragma pack()
```

On TriCore, 32-bit loads from unaligned addresses trigger a **misalignment trap** (Class 2,
TIN 5) unless the MEM_PROT configuration allows it. Even when allowed, unaligned access
requires two bus transactions, halving throughput.

**Memory Access Patterns**:
```c
/* Sequential access -- cache-friendly, prefetch-friendly */
void sequential_access(uint32 *data, uint32 n)
{
    for (uint32 i = 0; i < n; i++) {
        data[i] += 1;
    }
}

/* Strided access -- cache-hostile */
void strided_access(uint32 *data, uint32 n, uint32 stride)
{
    for (uint32 i = 0; i < n; i++) {
        data[(i * stride) % n] += 1;
    }
}

/* Random access -- worst case for cache */
void random_access(uint32 *data, uint32 *indices, uint32 n)
{
    for (uint32 i = 0; i < n; i++) {
        data[indices[i]] += 1;
    }
}
```

### 2.2 TriCore-Specific Throughput Features

#### 2.2.1 Load-Store Architecture

TriCore uses a strict **load-store architecture**: all arithmetic operates on registers, and
memory is accessed only through dedicated load/store instructions. This means:

- ALU operations never directly reference memory (unlike x86 CISC).
- The compiler must generate explicit LD/ST instructions around every computation.
- The LS pipeline can execute loads/stores in parallel with IP operations (dual issue).

This architecture favors keeping data in registers and operating on blocks of data loaded into
registers, rather than repeatedly loading/storing the same address.

#### 2.2.2 Hardware Loop Instructions

TriCore TC1.6.2 provides **zero-overhead hardware loop** support via the LOOP instruction:

```c
/* The compiler can emit LOOP instructions for simple counted loops.
 * With tricore-gcc, use -O2 or higher. The LOOP instruction
 * decrements a counter register and branches in zero cycles
 * (no branch penalty) when the loop is taken.
 *
 * tricore-gcc recognizes patterns like:
 */
void hw_loop_friendly(uint32 *dst, const uint32 *src, uint32 n)
{
    /* Simple counted loop -- tricore-gcc may emit LOOP instruction */
    for (uint32 i = n; i > 0; i--) {
        *dst++ = *src++;
    }
}

/* Force hardware loop with inline asm (if compiler doesn't) */
void hw_loop_asm(uint32 *dst, const uint32 *src, uint32 n)
{
    __asm__ volatile (
        "  mov.a  %%a2, %[src]        \n"
        "  mov.a  %%a3, %[dst]        \n"
        "  mov    %%d15, %[cnt]       \n"
        "1:                           \n"
        "  ld.w   %%d0, [%%a2+]4      \n"
        "  st.w   [%%a3+]4, %%d0      \n"
        "  loop   %%d15, 1b           \n"
        :
        : [src] "d" ((uint32)src), [dst] "d" ((uint32)dst), [cnt] "d" (n - 1)
        : "a2", "a3", "d0", "d15", "memory"
    );
}
```

The LOOP instruction eliminates the compare-and-branch overhead that consumes 1-2 cycles per
iteration in a normal loop. For very tight inner loops (2-3 instructions), this can improve
throughput by 20-40%.

---

## 3. Power Usage in Embedded CPUs

### 3.1 What Drives Dynamic Power Consumption

Dynamic power in CMOS circuits follows the equation:

    P_dynamic = alpha * C * V^2 * f

Where:
- **alpha** = switching activity factor (0 to 1)
- **C** = total switched capacitance
- **V** = supply voltage
- **f** = clock frequency

On the TC397, V and f are typically fixed for a given operating mode (300 MHz @ 1.25V typical).
The **switching activity factor (alpha)** is what code patterns can influence.

### 3.2 Factors Affecting Power Draw

#### 3.2.1 Switching Activity

Every transistor that toggles between 0 and 1 consumes energy. Code that causes more
register/bus bit toggles consumes more power:

- **High switching**: Alternating between 0x55555555 and 0xAAAAAAAA (all 32 bits toggle).
- **Low switching**: Operating on values that barely change (e.g., incrementing from 0).
- **Memory bus switching**: Each memory access drives address/data bus lines. Wide data
  transfers with high bit-toggle rates draw more current.

#### 3.2.2 Clock Gating and Sleep Modes

TC397 power modes:
- **Run mode**: All selected cores and peripherals active. Maximum power.
- **Idle mode**: Individual core clock stopped, peripherals still running. Entered via
  `IfxCpu_setCoreMode(cpu, IfxCpu_CoreMode_idle)`.
- **Sleep mode**: Core and most peripherals clock-gated. Wake on interrupt.
- **Standby mode**: Deepest sleep, only wake-up logic powered.

Peripheral clock gating:
```c
/* Disable unused peripheral clocks to reduce power */
/* Each peripheral has a CLC (Clock Control) register */
/* Example: Disable CAN module clock if unused */
MODULE_CAN0.CLC.B.DISR = 1; /* Request module disable */
```

#### 3.2.3 Memory Access Power

| Memory Type | Relative Power per Access |
|---|---|
| Register file | 1x (baseline) |
| DSPR/PSPR (scratch-pad) | ~2-3x |
| LMU/EMEM (shared SRAM) | ~4-6x |
| PFlash read | ~8-15x |
| DFlash read/write | ~20-40x |

Flash reads are particularly power-hungry because they require charge pump operation and
sense amplifier activation. Each flash bank activation draws significant current.

#### 3.2.4 Instruction-Type Power Differences

Different functional units have different power characteristics:

| Instruction Class | Relative Power | Notes |
|---|---|---|
| NOP | ~0.3x | Pipeline active but minimal switching |
| Simple ALU (ADD, MOV) | 1x (baseline) | |
| Multiply (MUL) | ~1.5-2x | Multiplier array activation |
| Divide (DVSTEP) | ~2-3x | Iterative divider, many internal toggles |
| MAC (MADD) | ~2x | Multiplier + accumulator |
| Floating point (Q31TOF, FTOQ31) | ~2-3x | FPU activation |
| Memory load (LD.W from DSPR) | ~1.5x | Address decode + SRAM read |
| Memory load (LD.W from PFlash) | ~3-5x | Flash bank activation |

### 3.3 Code Patterns Affecting Power Draw

#### 3.3.1 Tight Loops vs Scattered Memory Access

```c
/* LOW POWER: Tight register-only loop.
 * Data stays in registers, minimal memory bus activity. */
void low_power_compute(void)
{
    volatile uint32 result = 0;
    uint32 a = 0x12345678;
    uint32 b = 0x9ABCDEF0;
    for (uint32 i = 0; i < 10000; i++) {
        a = a ^ (a << 3);
        b = b ^ (b >> 5);
        a = a + b;
    }
    result = a;
}

/* HIGH POWER: Scattered memory access across multiple flash banks.
 * Activates flash banks, address bus toggles heavily, data bus toggles heavily. */
void high_power_memory(void)
{
    volatile uint32 result = 0;
    /* Access scattered addresses across different flash banks */
    volatile uint32 *addrs[] = {
        (volatile uint32 *)0x80000100, /* PFlash0 */
        (volatile uint32 *)0x80300100, /* PFlash1 */
        (volatile uint32 *)0x80600100, /* PFlash2 */
        (volatile uint32 *)0x80900100, /* PFlash3 */
        (volatile uint32 *)0x80C00100, /* PFlash4 */
    };
    uint32 sum = 0;
    for (uint32 i = 0; i < 10000; i++) {
        sum += *addrs[i % 5]; /* Activates 5 different flash banks */
    }
    result = sum;
}
```

#### 3.3.2 NOP-Heavy Code vs Compute-Heavy Code

```c
/* NOP-heavy: Pipeline runs but minimal functional unit switching.
 * Still consumes clock tree power, but low data-path switching. */
void nop_heavy(uint32 iterations)
{
    for (uint32 i = 0; i < iterations; i++) {
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
    }
}

/* Compute-heavy: Maximum functional unit switching.
 * Multiplier, ALU, shifter all active every cycle. */
void compute_heavy(uint32 iterations)
{
    volatile uint32 r = 0;
    uint32 a = 0xDEADBEEF, b = 0xCAFEBABE, c = 0x13579BDF;
    for (uint32 i = 0; i < iterations; i++) {
        a = a * b + c;       /* MUL + ADD */
        b = (b << 7) ^ a;   /* SH + XOR */
        c = c + (a >> 3);   /* SH + ADD */
        a = a ^ (b & c);    /* AND + XOR */
    }
    r = a + b + c;
}
```

#### 3.3.3 DSP Instructions vs Generic ALU

```c
/* Generic ALU: Simple adds and shifts */
void generic_filter(const int16_t *input, int16_t *output, uint32 n)
{
    for (uint32 i = 1; i < n - 1; i++) {
        int32_t sum = (int32_t)input[i-1] + 2 * (int32_t)input[i]
                    + (int32_t)input[i+1];
        output[i] = (int16_t)(sum >> 2);
    }
}

/* DSP-style: Uses multiply-accumulate, saturation, packed operations.
 * Higher switching activity in the multiplier array = higher power. */
void dsp_filter(const int16_t *input, int16_t *output, uint32 n)
{
    for (uint32 i = 1; i < n - 1; i++) {
        /* Using TriCore MAC instruction via intrinsic */
        int32_t acc = 0;
        __asm__ volatile (
            "madd %0, %0, %1, %2"
            : "+d" (acc)
            : "d" ((int32_t)input[i-1]), "d" (1)
        );
        __asm__ volatile (
            "madd %0, %0, %1, %2"
            : "+d" (acc)
            : "d" ((int32_t)input[i]), "d" (2)
        );
        __asm__ volatile (
            "madd %0, %0, %1, %2"
            : "+d" (acc)
            : "d" ((int32_t)input[i+1]), "d" (1)
        );
        output[i] = (int16_t)(acc >> 2);
    }
}
```

---

## 4. TriCore TC397 Specifics

### 4.1 AURIX TC397 Architecture Overview

The TC397 (AURIX 2G) contains:

| Component | Details |
|---|---|
| CPU Cores | 3x TC1.6.2P (CPU0, CPU1, CPU2) + 3x TC1.6.2E (CPU3, CPU4, CPU5) |
| Max Frequency | 300 MHz |
| Program Flash (PFlash) | 6 banks, ~16 MB total |
| Data Flash (DFlash) | 1 MB |
| DSPR per core | 240 KB (CPU0, CPU1), 96 KB (CPU2-CPU5) |
| PSPR per core | 64 KB each |
| LMU/DLMU | 64 KB per CPU (DLMU) + 768 KB shared (LMU) |
| EMEM | 4 MB (Extended Memory) |
| PCACHE | 16 KB per core (2-way set-associative) |
| DCACHE | CPU0/CPU1 only, 16 KB |

CPU0/CPU1/CPU2 are "performance" cores (TC1.6.2P) with dual-issue capability.
CPU3/CPU4/CPU5 are "efficiency" cores (TC1.6.2E) with reduced capabilities.

### 4.2 Memory Map (from project linker script)

Based on the project's `Lcf_Gnuc_Tricore_Tc.lsl`:

```
Segment 0x1xxxxxxx: DSPR5 (96K), PSPR5 (64K)
Segment 0x3xxxxxxx: DSPR4 (96K), PSPR4 (64K)
Segment 0x4xxxxxxx: DSPR3 (96K), PSPR3 (64K)
Segment 0x5xxxxxxx: DSPR2 (96K), PSPR2 (64K)
Segment 0x6xxxxxxx: DSPR1 (240K), PSPR1 (64K)
Segment 0x7xxxxxxx: DSPR0 (240K), PSPR0 (64K)

Segment 0x8xxxxxxx: PFlash (cached) -- 6 banks of 1-3 MB
Segment 0x9xxxxxxx: LMU/DLMU (cached)
Segment 0xAxxxxxxx: PFlash (non-cached mirror)
Segment 0xBxxxxxxx: LMU (non-cached mirror)
Segment 0xCxxxxxxx: PSPR (local alias)
Segment 0xDxxxxxxx: DSPR (local alias)
Segment 0xFxxxxxxx: Peripheral registers
```

The local aliases (0xC0000000 for PSPR, 0xD0000000 for DSPR) are critical: each core sees
its own scratch-pad at the same local address, but the global address differs per core. The
iLLD macros `IFXCPU_GLB_ADDR_DSPR()` and `IFXCPU_GLB_ADDR_PSPR()` handle this translation.

### 4.3 Scratch-Pad RAM (DSPR/PSPR)

Scratch-pad RAM is **not a cache** -- it is directly mapped, deterministic SRAM:

- **DSPR** (Data Scratch-Pad RAM): For data. Zero wait state when accessed by the owning core.
  Accessible by other cores via global address, but with SRI arbitration latency (2-6 cycles).
- **PSPR** (Program Scratch-Pad RAM): For executable code. Zero wait state when the owning
  core fetches instructions from it.

Placing time-critical code in PSPR:
```c
/* In C source, use section attribute */
__attribute__((section(".psram_cpu0")))
void critical_isr(void)
{
    /* This function will be placed in CPU0's PSPR by the linker */
    /* Zero wait state instruction fetch -- fully deterministic timing */
}
```

In the linker script, the corresponding section maps to `psram0`:
```
.psram_cpu0 : { *(.psram_cpu0) } > psram0
```

Placing time-critical data in DSPR:
```c
/* Data in CPU0's local DSPR */
static uint32 critical_data[256] __attribute__((section(".data_cpu0")));
```

### 4.4 Flash Wait States

PFlash wait states on TC397 depend on:
- CPU frequency (fCPU) relative to flash frequency (fFLASH).
- Flash access width: 256-bit (burst read fills an entire cache line).
- Sequential vs. random access: sequential reads from flash can benefit from prefetch.

Typical configuration at 300 MHz:
- **PFlash wait states**: 5-6 (configured in FLASH0_FCON register).
- **Cache line fill**: One 256-bit read can take ~8-10 CPU cycles total.
- **Cache hit**: 0 additional wait states (served from PCACHE in 1 cycle).

The implication: **code that loops tightly within a 16KB working set will be served entirely
from cache and run at near-PSPR speed. Code that jumps around a >16KB code region will
suffer repeated cache misses.**

### 4.5 SRI Crossbar Arbitration

The SRI (Shared Resource Interconnect) is a crossbar switch that connects bus masters
(CPUs, DMA) to bus slaves (memory targets, peripherals). Key characteristics:

- Each slave port has an independent arbiter.
- Arbitration can be round-robin or fixed-priority per slave.
- A CPU accessing another core's DSPR goes through the SRI -- adding 2-4 cycles latency.
- Multiple masters accessing the same slave simultaneously results in serialized access
  (one wins, others stall).

**Multicore contention test pattern**:
```c
/* All cores hammer the same LMU address range -- maximum contention */
void contention_test(uint32 core_id)
{
    volatile uint32 *shared_mem = (volatile uint32 *)0xB0040000; /* LMU non-cached */
    for (uint32 i = 0; i < 100000; i++) {
        shared_mem[core_id] = i; /* Write to LMU */
        (void)shared_mem[0];     /* Read from LMU -- contention with other cores */
    }
}
```

### 4.6 Hardware Loop Instructions

The TriCore LOOP instruction decrements an address register and branches if non-zero, with
**zero branch penalty** (it is architecturally predicted-taken and folded into the pipeline):

```
LOOP An, target      ; An = An - 1; if (An != 0) goto target
LOOPU target          ; Unconditional zero-overhead loop (infinite until explicit exit)
```

The compiler may generate these for simple for/while loops with -O2 and higher. The benefit
is eliminating the 1-2 cycle branch penalty on every loop iteration.

### 4.7 Saturation Arithmetic

TriCore provides dedicated saturation instructions that clamp results to min/max values
without branching:

```c
/* Without saturation (requires conditional branches) */
int32_t clamp_generic(int32_t x, int32_t lo, int32_t hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/* With TriCore saturation intrinsics (single instruction, no branches) */
#include "IfxCpu_Intrinsics.h"

int16_t saturate_to_s16(int32_t x)
{
    /* SATS.H instruction: saturate 32-bit to signed 16-bit range */
    int16_t result;
    __asm__ volatile ("sats.h %0, %1" : "=d"(result) : "d"(x));
    return result;
}

uint8_t saturate_to_u8(int32_t x)
{
    /* SAT.BU instruction: saturate to unsigned 8-bit range */
    uint8_t result;
    __asm__ volatile ("sat.bu %0, %1" : "=d"(result) : "d"(x));
    return result;
}
```

### 4.8 Packed Operations

TriCore supports SIMD-like packed operations on 32-bit registers containing 2x16-bit or
4x8-bit values:

```c
/* Process 2 halfwords simultaneously */
void packed_halfword_add(int16_t *a, int16_t *b, int16_t *c, uint32 n)
{
    /* Process two elements at once using packed halfword operations */
    uint32 *pa = (uint32 *)a;
    uint32 *pb = (uint32 *)b;
    uint32 *pc = (uint32 *)c;
    for (uint32 i = 0; i < n / 2; i++) {
        uint32 va = pa[i];
        uint32 vb = pb[i];
        uint32 vc;
        __asm__ volatile (
            "add.h %0, %1, %2"  /* Packed halfword add: c[0:15]=a[0:15]+b[0:15], c[16:31]=a[16:31]+b[16:31] */
            : "=d"(vc) : "d"(va), "d"(vb)
        );
        pc[i] = vc;
    }
}

/* Process 4 bytes simultaneously */
void packed_byte_abs(int8_t *data, uint32 n)
{
    uint32 *p = (uint32 *)data;
    for (uint32 i = 0; i < n / 4; i++) {
        uint32 v = p[i];
        uint32 r;
        __asm__ volatile (
            "abs.b %0, %1"  /* Packed byte absolute value: 4 bytes at once */
            : "=d"(r) : "d"(v)
        );
        p[i] = r;
    }
}
```

Available packed operations include: ADD.H, SUB.H, ABS.H, ABSS.H, MIN.H, MAX.H, ADD.B,
ABS.B, and corresponding unsigned variants.

### 4.9 Circular Buffers

TriCore has hardware support for circular buffer addressing via address register pairs. This
is used for DSP-style ring buffer operations without explicit modulo arithmetic:

```c
/* Software circular buffer (generic approach) */
void circular_buffer_sw(int16_t *buffer, uint32 size, int16_t new_val, int32_t *sum)
{
    static uint32 index = 0;
    *sum -= buffer[index];
    buffer[index] = new_val;
    *sum += new_val;
    index = (index + 1) % size; /* Modulo operation -- expensive without HW support */
}

/* Hardware circular buffer using TriCore address register pair.
 * The A[n+1]:A[n] register pair holds base+index, and the circular
 * addressing mode automatically wraps. This requires inline asm. */
/* Note: tricore-gcc does not expose circular addressing in C directly.
 * It must be done via inline assembly or compiler intrinsics. */
```

### 4.10 What Makes TriCore Unique vs ARM/x86 for Performance Testing

| Feature | TriCore TC397 | ARM Cortex-R/M | x86 |
|---|---|---|---|
| Branch prediction | Static only | Dynamic (Cortex-R5+) | Advanced dynamic |
| Out-of-order exec | No | No (Cortex-R) | Yes |
| Cache | Small (16KB), deterministic | Varies, often larger | Large, multi-level |
| Scratch-pad RAM | Yes (DSPR/PSPR), key feature | TCM (similar) | No equivalent |
| Hardware loops | LOOP instruction | No HW loop (Cortex-M) | No HW loop |
| Packed ops | 2x16b, 4x8b in 32b regs | NEON (128b vectors) | SSE/AVX |
| Saturation | Native instructions | QADD/QSUB (Cortex-M) | SSE2+ |
| Multicore coherence | No cache coherence | Optional (Cortex-A) | MESI/MOESI |
| Memory model | Strongly ordered (mostly) | Weakly ordered (ARM) | x86-TSO |
| Context save | Hardware CSA | Software (push/pop) | Software |
| Interrupt latency | ~10-15 cycles (HW context) | ~12 cycles (Cortex-M) | ~100+ cycles |

**Key differentiators for performance testing**:
1. **No cache coherence**: Cross-core data sharing requires explicit cache management (flush/
   invalidate) or use of non-cached memory regions. This makes multicore contention patterns
   very visible.
2. **Deterministic scratch-pad**: DSPR/PSPR give perfectly predictable timing, making it easy
   to establish precise baselines and isolate cache effects.
3. **No dynamic branch prediction**: Branch-heavy code pays a predictable penalty every time
   a forward branch is taken. No warm-up effects from branch predictors.
4. **Small caches**: The 16KB PCACHE means cache thrashing happens with relatively small
   working sets (>16KB). This is easy to trigger deliberately.

---

## 5. Code Patterns That Create Measurable Binary Differences

### 5.1 Execution Speed Differences

#### 5.1.1 Hot Loop -- Register-Resident vs Memory-Resident

```c
/* FAST: All computation in registers, data in DSPR */
__attribute__((section(".psram_cpu0")))
uint32 hot_loop_fast(void)
{
    /* Data in local DSPR -- 0 wait state */
    static uint32 data[256] __attribute__((section(".data_cpu0")));
    uint32 sum = 0;
    for (uint32 i = 0; i < 256; i++) {
        sum += data[i] * (i + 1); /* MUL + ADD, both in registers after load */
    }
    return sum;
}

/* SLOW: Same computation, but data in non-cached flash */
uint32 hot_loop_slow(void)
{
    /* Data in DFlash -- 10-20+ wait states per access */
    volatile uint32 *data = (volatile uint32 *)0xAF000000;
    uint32 sum = 0;
    for (uint32 i = 0; i < 256; i++) {
        sum += data[i] * (i + 1);
    }
    return sum;
}
```

Expected difference: 5-20x cycle count difference.

#### 5.1.2 Function Call Overhead -- Inlined vs Non-Inlined

```c
/* INLINED: Compiler eliminates call/return overhead */
static inline __attribute__((always_inline))
uint32 add_inline(uint32 a, uint32 b)
{
    return a + b;
}

/* NOT INLINED: Each call saves/restores context via CSA */
__attribute__((noinline))
uint32 add_noinline(uint32 a, uint32 b)
{
    return a + b;
}

/* Test harness */
uint32 test_call_overhead(uint32 choice)
{
    uint32 sum = 0;
    if (choice == 0) {
        /* Inlined: no function call overhead. ~1 cycle per add. */
        for (uint32 i = 0; i < 10000; i++) {
            sum = add_inline(sum, i);
        }
    } else {
        /* Non-inlined: CALL + RET + context save/restore overhead.
         * Each call costs ~6-10 additional cycles on TriCore due to
         * CSA (Context Save Area) management. */
        for (uint32 i = 0; i < 10000; i++) {
            sum = add_noinline(sum, i);
        }
    }
    return sum;
}
```

Expected difference: ~5-10x for trivial functions (call overhead dominates).

#### 5.1.3 Branch-Heavy vs Branch-Free Code

```c
/* BRANCH-HEAVY: Many unpredictable forward branches.
 * Each taken forward branch costs 2-3 cycles (misprediction). */
uint32 branch_heavy(const uint32 *data, uint32 n)
{
    uint32 count = 0;
    for (uint32 i = 0; i < n; i++) {
        if (data[i] > 100) {       /* Forward branch, predicted not-taken */
            count++;
        } else if (data[i] > 50) { /* Another forward branch */
            count += 2;
        } else if (data[i] > 25) { /* Yet another */
            count += 3;
        } else {
            count += 4;
        }
    }
    return count;
}

/* BRANCH-FREE: Same logic using conditional moves/arithmetic.
 * No branches, no misprediction penalties. */
uint32 branch_free(const uint32 *data, uint32 n)
{
    uint32 count = 0;
    for (uint32 i = 0; i < n; i++) {
        uint32 v = data[i];
        /* Use arithmetic to compute the increment without branches */
        uint32 gt100 = (v > 100) ? 1 : 0;
        uint32 gt50  = (v > 50 && v <= 100) ? 1 : 0;
        uint32 gt25  = (v > 25 && v <= 50) ? 1 : 0;
        uint32 le25  = (v <= 25) ? 1 : 0;
        count += gt100 * 1 + gt50 * 2 + gt25 * 3 + le25 * 4;
    }
    return count;
}

/* Even more branch-free using TriCore's conditional select.
 * tricore-gcc will use SEL/SELN instructions with -O2. */
uint32 branch_free_sel(const uint32 *data, uint32 n)
{
    uint32 count = 0;
    for (uint32 i = 0; i < n; i++) {
        uint32 v = data[i];
        /* TriCore has SEL instruction: SEL Dc, Da, Db (if D15 then Da else Db) */
        uint32 inc = 4;
        inc = (v > 25)  ? 3 : inc;
        inc = (v > 50)  ? 2 : inc;
        inc = (v > 100) ? 1 : inc;
        count += inc;
    }
    return count;
}
```

Expected difference: 20-40% cycle count difference with random input data.

### 5.2 Binary Size Differences

#### 5.2.1 Inlining Aggressiveness

```c
/* Compile with -finline-limit=0 (no inlining) vs -finline-limit=10000 (aggressive inlining) */

/* Small helper function called from many places */
uint32 compute_checksum(const uint8 *data, uint32 len)
{
    uint32 csum = 0;
    for (uint32 i = 0; i < len; i++) {
        csum = (csum << 1) ^ data[i];
    }
    return csum;
}

/* 20 call sites -- inlining all of them dramatically increases binary size */
void process_packet_0(const uint8 *p) { volatile uint32 r = compute_checksum(p, 64); }
void process_packet_1(const uint8 *p) { volatile uint32 r = compute_checksum(p, 128); }
void process_packet_2(const uint8 *p) { volatile uint32 r = compute_checksum(p, 256); }
/* ... up to process_packet_19 ... */
```

With inlining: each call site gets a copy of the loop body. Binary grows by ~100-200 bytes
per call site.
Without inlining: single copy of `compute_checksum`, each call site is just a CALL instruction.

**Binary size impact**: ~500 bytes (no inline) vs ~4000+ bytes (full inline).
**Speed impact**: Inlined version is faster due to no call overhead and possible constant
propagation (e.g., the `len` parameter can be folded into the loop count).

#### 5.2.2 Dead Code Elimination

```c
/* tricore-gcc -ffunction-sections -fdata-sections + linker --gc-sections
 * removes unused functions and data. */

/* This function is never called -- with gc-sections, it gets removed */
__attribute__((used)) /* Prevent removal -- forces it into binary */
void dead_code_large(void)
{
    static const uint32 large_table[4096] = { /* 16KB of data */ };
    volatile uint32 r = large_table[0];
    (void)r;
}

/* Without __attribute__((used)) and with -ffunction-sections + --gc-sections,
 * this entire function and its table are removed, saving 16KB+. */
```

#### 5.2.3 LUT vs Computation Tradeoffs

```c
/* LARGE BINARY: Lookup table approach.
 * Fast execution but large binary size. */
static const uint16 sin_lut[4096] = {
    /* 4096 entries * 2 bytes = 8KB in flash */
    0, 100, 201, 301, 402, 502, /* ... */
};

uint16 sin_lut_lookup(uint16 angle)
{
    return sin_lut[angle & 0x0FFF];
}

/* SMALL BINARY: Computation approach.
 * Slow execution but tiny binary size. */
int16_t sin_compute(uint16 angle)
{
    /* CORDIC or polynomial approximation */
    /* Taylor series: sin(x) ~ x - x^3/6 + x^5/120 */
    float x = (float)angle * (2.0f * 3.14159265f / 4096.0f);
    float x2 = x * x;
    float result = x * (1.0f - x2 * (1.0f/6.0f - x2 * (1.0f/120.0f)));
    return (int16_t)(result * 32767.0f);
}
```

**Binary size**: LUT version adds 8KB; computation version adds ~100-200 bytes.
**Execution time**: LUT is 1-2 cycles (single memory read); computation is 30-80 cycles.

### 5.3 Power Consumption Differences

#### 5.3.1 NOP Insertion (Low Activity)

```c
/* Minimum power while still "running": NOP loop.
 * Clock tree still switching, but data path is quiet. */
void power_minimal_active(uint32 duration_ticks)
{
    uint32 start = IfxStm_getLower(&MODULE_STM0);
    while ((IfxStm_getLower(&MODULE_STM0) - start) < duration_ticks) {
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
        __asm__ volatile ("nop");
    }
}
```

#### 5.3.2 Maximum Data Path Switching

```c
/* Maximum switching activity: alternate between all-ones and all-zeros
 * patterns across as many bus lines and registers as possible. */
void power_max_switching(uint32 iterations)
{
    volatile uint32 *lmu_nc = (volatile uint32 *)0xB0040000;
    uint32 pattern_a = 0xFFFFFFFF;
    uint32 pattern_b = 0x00000000;
    for (uint32 i = 0; i < iterations; i++) {
        /* Write alternating patterns to non-cached LMU
         * to maximize data bus toggling */
        lmu_nc[0] = pattern_a;
        lmu_nc[1] = pattern_b;
        lmu_nc[2] = pattern_a;
        lmu_nc[3] = pattern_b;
        /* Also toggle address bus by accessing far-apart addresses */
        lmu_nc[0x100] = pattern_b;
        lmu_nc[0x101] = pattern_a;
        /* Swap patterns */
        uint32 tmp = pattern_a;
        pattern_a = pattern_b;
        pattern_b = tmp;
    }
}
```

#### 5.3.3 Peripheral Toggling (GPIO Stress)

```c
/* Toggle GPIO pins at maximum rate -- drives I/O pad switching power */
void power_gpio_stress(uint32 iterations)
{
    /* Assuming Port 00, Pin 5 is configured as output */
    volatile Ifx_P *port = &MODULE_P00;
    for (uint32 i = 0; i < iterations; i++) {
        port->OMR.U = (1 << 5);            /* Set pin */
        port->OMR.U = (1 << (5 + 16));     /* Clear pin */
    }
}
```

### 5.4 Throughput Differences

#### 5.4.1 Scalar vs Packed Operations

```c
/* SCALAR: Process one element at a time */
void vector_add_scalar(const int16_t *a, const int16_t *b, int16_t *c, uint32 n)
{
    for (uint32 i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

/* PACKED: Process two elements at a time using TriCore packed halfword ops */
void vector_add_packed(const int16_t *a, const int16_t *b, int16_t *c, uint32 n)
{
    const uint32 *pa = (const uint32 *)a;
    const uint32 *pb = (const uint32 *)b;
    uint32 *pc = (uint32 *)c;
    uint32 half_n = n / 2;

    for (uint32 i = 0; i < half_n; i++) {
        uint32 va = pa[i];
        uint32 vb = pb[i];
        uint32 vc;
        /* ADD.H: adds two pairs of 16-bit halfwords in parallel */
        __asm__ volatile ("add.h %0, %1, %2" : "=d"(vc) : "d"(va), "d"(vb));
        pc[i] = vc;
    }

    /* Handle odd element if n is odd */
    if (n & 1) {
        c[n - 1] = a[n - 1] + b[n - 1];
    }
}
```

Expected throughput difference: ~1.8-2x for the packed version.

#### 5.4.2 Memory Copy Optimization

```c
#include <string.h>

/* BASELINE: Byte-by-byte copy */
void memcpy_byte(void *dst, const void *src, uint32 n)
{
    uint8 *d = (uint8 *)dst;
    const uint8 *s = (const uint8 *)src;
    for (uint32 i = 0; i < n; i++) {
        d[i] = s[i]; /* LD.BU + ST.B: one byte per cycle */
    }
}

/* OPTIMIZED: Word-aligned 32-bit copy */
void memcpy_word(void *dst, const void *src, uint32 n)
{
    uint32 *d = (uint32 *)dst;
    const uint32 *s = (const uint32 *)src;
    uint32 words = n / 4;
    for (uint32 i = 0; i < words; i++) {
        d[i] = s[i]; /* LD.W + ST.W: 4 bytes per cycle */
    }
    /* Copy remaining bytes */
    uint8 *db = (uint8 *)&d[words];
    const uint8 *sb = (const uint8 *)&s[words];
    for (uint32 i = 0; i < (n & 3); i++) {
        db[i] = sb[i];
    }
}

/* HIGHLY OPTIMIZED: Double-word copy with loop unrolling */
void memcpy_doubleword(void *dst, const void *src, uint32 n)
{
    uint64 *d = (uint64 *)dst;
    const uint64 *s = (const uint64 *)src;
    uint32 dwords = n / 8;
    for (uint32 i = 0; i < dwords; i++) {
        d[i] = s[i]; /* LD.D + ST.D: 8 bytes per iteration */
    }
    /* Handle remaining bytes */
    uint8 *db = (uint8 *)&d[dwords];
    const uint8 *sb = (const uint8 *)&s[dwords];
    for (uint32 i = 0; i < (n & 7); i++) {
        db[i] = sb[i];
    }
}
```

Expected throughput: byte copy = 1 byte/cycle; word copy = 4 bytes/cycle; double-word copy
= 8 bytes/cycle (if memory bandwidth allows). That is a 4-8x throughput difference.

---

## 6. Stress Test Patterns

### 6.1 Matrix Multiplication

Matrix multiplication is the classic compute-intensive benchmark. It stresses the multiply
unit, has regular memory access patterns, and scales well.

```c
#define MAT_SIZE 32

/* Naive matrix multiply: O(n^3), poor cache behavior for large matrices */
void matmul_naive(const float *A, const float *B, float *C, uint32 n)
{
    for (uint32 i = 0; i < n; i++) {
        for (uint32 j = 0; j < n; j++) {
            float sum = 0.0f;
            for (uint32 k = 0; k < n; k++) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
        }
    }
}

/* Cache-friendly matrix multiply: transpose B first, then row-row access.
 * Significantly better for cache utilization. */
void matmul_transposed(const float *A, const float *B, float *C, uint32 n)
{
    /* Transpose B into BT */
    float BT[MAT_SIZE * MAT_SIZE];
    for (uint32 i = 0; i < n; i++) {
        for (uint32 j = 0; j < n; j++) {
            BT[i * n + j] = B[j * n + i];
        }
    }
    /* Now both A and BT are accessed row-wise (sequential) */
    for (uint32 i = 0; i < n; i++) {
        for (uint32 j = 0; j < n; j++) {
            float sum = 0.0f;
            for (uint32 k = 0; k < n; k++) {
                sum += A[i * n + k] * BT[j * n + k];
            }
            C[i * n + j] = sum;
        }
    }
}

/* Integer fixed-point version using TriCore MAC for maximum throughput */
void matmul_fixedpoint(const int16_t *A, const int16_t *B, int32_t *C, uint32 n)
{
    for (uint32 i = 0; i < n; i++) {
        for (uint32 j = 0; j < n; j++) {
            int32_t acc = 0;
            for (uint32 k = 0; k < n; k++) {
                /* TriCore MADD instruction: acc += A[i][k] * B[k][j] */
                acc += (int32_t)A[i * n + k] * (int32_t)B[k * n + j];
            }
            C[i * n + j] = acc;
        }
    }
}
```

**Effectiveness for revealing differences**: HIGH. Matrix size controls working set size,
exposing cache behavior. The naive vs transposed versions differ by 2-5x depending on matrix
size relative to cache size. Float vs integer versions expose FPU vs integer throughput.

### 6.2 FFT (Fast Fourier Transform)

The project already includes an FFT implementation in the iLLD library
(`Ifx_FftF32.c`). FFT is excellent for stress testing because:

- **Butterfly operations**: Mix of multiply, add, subtract on complex numbers.
- **Bit-reversal permutation**: Random-looking memory access pattern.
- **Twiddle factor lookups**: Table access pattern.
- **Working set scales with FFT size**: Easy to control cache pressure.

```c
#include "Ifx_FftF32.h"

/* Use the existing iLLD FFT for benchmarking */
void fft_stress_test(uint32 iterations)
{
    cfloat32 input[256];
    cfloat32 output[256];

    /* Initialize with test signal */
    for (uint32 i = 0; i < 256; i++) {
        input[i].real = (float32)(i % 64);
        input[i].imag = 0.0f;
    }

    IfxCpu_resetAndStartCounters(IfxCpu_CounterMode_normal);
    for (uint32 iter = 0; iter < iterations; iter++) {
        Ifx_FftF32_radix2(output, input, 256);
    }
    IfxCpu_Perf perf = IfxCpu_stopCounters();
    /* perf.clock.counter contains total cycles */
}
```

**Effectiveness for revealing differences**: HIGH. Different FFT sizes expose different cache
behaviors. Radix-2 vs radix-4 implementations have different instruction mixes.

### 6.3 CRC Computation

The project includes both software CRC (in `Ifx_Crc.c`) and a hardware CRC engine (FCE
module in `IfxFce_Crc.c`). CRC computation is an excellent benchmark because:

- **Table-based CRC**: Tests memory access patterns (256-entry lookup table).
- **Bit-by-bit CRC**: Tests branch-heavy, shift-heavy computation.
- **Hardware CRC**: Tests peripheral offloading vs CPU computation.

```c
#include "Ifx_Crc.h"

/* Software CRC-32 (table-based) vs bit-by-bit */
void crc_stress_test(uint8 *data, uint32 len, uint32 iterations)
{
    /* Create CRC table (one-time setup) */
    typedef struct {
        Ifc_Crc_Table data;
        uint32 crctab[256];
    } Ifc_Crc_Table32;

    Ifc_Crc_Table32 table32;
    Ifc_Crc driver;

    Ifx_Crc_createTable(&table32.data, 32, 0x04C11DB7, 0);
    Ifx_Crc_init(&driver, &table32.data, 1, 1, 0xFFFFFFFF, 0xFFFFFFFF);

    volatile uint32 result;

    /* Table-based (fast, but needs LUT in cache/DSPR) */
    IfxCpu_resetAndStartCounters(IfxCpu_CounterMode_normal);
    for (uint32 i = 0; i < iterations; i++) {
        result = Ifx_Crc_tableFast(&driver, data, len);
    }
    IfxCpu_Perf perf_table = IfxCpu_stopCounters();

    /* Bit-by-bit (slow, but no LUT needed, smaller binary) */
    IfxCpu_resetAndStartCounters(IfxCpu_CounterMode_normal);
    for (uint32 i = 0; i < iterations; i++) {
        result = Ifx_Crc_bitByBitFast(&driver, data, len);
    }
    IfxCpu_Perf perf_bitbybit = IfxCpu_stopCounters();
}
```

**Effectiveness for revealing differences**: HIGH. Table-based vs bit-by-bit CRC creates
large speed differences (10-50x). The table version's performance depends heavily on whether
the LUT fits in cache/DSPR.

### 6.4 Sorting Algorithms

Different sorting algorithms create very different execution profiles:

```c
/* Quicksort: Branch-heavy, cache-unfriendly for random data,
 * function call overhead from recursion. */
void quicksort(uint32 *arr, int32_t lo, int32_t hi)
{
    if (lo >= hi) return;
    uint32 pivot = arr[hi];
    int32_t i = lo - 1;
    for (int32_t j = lo; j < hi; j++) {
        if (arr[j] <= pivot) {
            i++;
            uint32 tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }
    uint32 tmp = arr[i + 1];
    arr[i + 1] = arr[hi];
    arr[hi] = tmp;
    int32_t p = i + 1;
    quicksort(arr, lo, p - 1);
    quicksort(arr, p + 1, hi);
}

/* Insertion sort: Simple, cache-friendly (sequential access),
 * but O(n^2) complexity. Good for small arrays. */
void insertion_sort(uint32 *arr, uint32 n)
{
    for (uint32 i = 1; i < n; i++) {
        uint32 key = arr[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/* Radix sort: No comparisons, pure memory operations.
 * Tests memory bandwidth rather than branch prediction. */
void radix_sort_byte(uint32 *arr, uint32 n)
{
    uint32 output[1024]; /* Temp buffer */
    uint32 count[256];

    for (uint32 byte_pos = 0; byte_pos < 4; byte_pos++) {
        /* Reset counts */
        for (uint32 i = 0; i < 256; i++) count[i] = 0;

        /* Count occurrences */
        for (uint32 i = 0; i < n; i++) {
            uint32 byte_val = (arr[i] >> (byte_pos * 8)) & 0xFF;
            count[byte_val]++;
        }

        /* Cumulative count */
        for (uint32 i = 1; i < 256; i++) count[i] += count[i - 1];

        /* Build output */
        for (int32_t i = (int32_t)n - 1; i >= 0; i--) {
            uint32 byte_val = (arr[i] >> (byte_pos * 8)) & 0xFF;
            output[count[byte_val] - 1] = arr[i];
            count[byte_val]--;
        }

        /* Copy back */
        for (uint32 i = 0; i < n; i++) arr[i] = output[i];
    }
}
```

**Effectiveness for revealing differences**: MEDIUM-HIGH. Quicksort vs insertion sort shows
algorithm-level differences. Quicksort recursion stresses the CSA (context save area).
Radix sort stresses memory bandwidth.

### 6.5 Memory Copy/Fill Patterns

```c
/* Memory fill with different patterns to test bus utilization */
void memfill_stress(volatile uint32 *target, uint32 size_words, uint32 pattern)
{
    for (uint32 i = 0; i < size_words; i++) {
        target[i] = pattern;
    }
}

/* DMA-based memory copy for comparison */
#include "IfxDma_Dma.h"

void dma_memcpy_setup(uint32 *dst, const uint32 *src, uint32 word_count)
{
    /* Configure a DMA channel for memory-to-memory transfer */
    IfxDma_Dma_Channel dmaCh;
    IfxDma_Dma_ChannelConfig dmaCfg;

    IfxDma_Dma_initChannelConfig(&dmaCfg, &MODULE_DMA);
    dmaCfg.sourceAddress = (uint32)src;
    dmaCfg.destinationAddress = (uint32)dst;
    dmaCfg.transferCount = word_count;
    dmaCfg.moveSize = IfxDma_ChannelMoveSize_32bit;
    dmaCfg.blockMode = IfxDma_ChannelMove_1;

    IfxDma_Dma_initChannel(&dmaCh, &dmaCfg);
    IfxDma_Dma_startChannelTransaction(&dmaCh);
    /* Wait for completion */
    while (IfxDma_Dma_isChannelTransactionPending(&dmaCh)) {}
}
```

**Effectiveness**: MEDIUM. Good for measuring raw memory bandwidth and DMA vs CPU copy
performance. Reveals SRI contention patterns when run on multiple cores.

### 6.6 Interrupt Storm Generation

```c
/* Generate high-frequency interrupts using STM (System Timer) */
#include "IfxStm.h"
#include "IfxSrc.h"

/* Configurable interrupt rate */
#define ISR_PERIOD_TICKS 100 /* At 100 MHz STM clock = 1 MHz interrupt rate */

/* ISR counter for measuring response */
volatile uint32 isr_count = 0;
volatile uint32 isr_worst_latency = 0;

/* STM interrupt handler */
IFX_INTERRUPT(stm0_isr, 0, 10) /* CPU0, priority 10 */
{
    uint32 entry_time = IfxStm_getLower(&MODULE_STM0);

    /* Clear interrupt and set next compare */
    IfxStm_clearCompareFlag(&MODULE_STM0, IfxStm_Comparator_0);
    uint32 next = IfxStm_getCompare(&MODULE_STM0, IfxStm_Comparator_0)
                + ISR_PERIOD_TICKS;
    IfxStm_setCompare(&MODULE_STM0, IfxStm_Comparator_0, next);

    /* Measure latency (time from compare match to ISR entry) */
    uint32 latency = entry_time - (next - ISR_PERIOD_TICKS);
    if (latency > isr_worst_latency) {
        isr_worst_latency = latency;
    }
    isr_count++;
}

/* Background task running during interrupt storm */
void background_task_under_storm(void)
{
    volatile uint32 result = 0;
    uint32 sum = 0;
    /* This loop will be frequently preempted by the ISR */
    for (uint32 i = 0; i < 1000000; i++) {
        sum += i * i;
    }
    result = sum;
}
```

**Effectiveness**: HIGH. Measures worst-case interrupt latency under load, which is a critical
metric for automotive/safety applications. The background task's throughput degradation under
interrupt storm reveals interrupt overhead.

### 6.7 Multicore Stress Test

```c
/* Cross-core contention test: all cores compete for shared resources */

/* Shared data in LMU (accessible by all cores) */
static volatile uint32 shared_counter
    __attribute__((section(".lmuram"))) = 0;

static volatile uint32 core_results[6]
    __attribute__((section(".lmuram")));

/* Spinlock for atomic access */
static IfxCpu_spinLock shared_lock
    __attribute__((section(".lmuram"))) = 0;

/* Function run by each core */
void multicore_stress(uint32 core_id, uint32 iterations)
{
    uint32 local_sum = 0;

    for (uint32 i = 0; i < iterations; i++) {
        /* Contention point 1: Atomic increment with spinlock */
        if (IfxCpu_setSpinLock(&shared_lock, 1000)) {
            shared_counter++;
            IfxCpu_resetSpinLock(&shared_lock);
        }

        /* Contention point 2: All cores read same LMU location */
        local_sum += shared_counter;

        /* Local work (no contention) in own DSPR */
        local_sum ^= (local_sum << 3) | (local_sum >> 29);
    }

    core_results[core_id] = local_sum;
}
```

**Effectiveness**: VERY HIGH. This test exposes:
- SRI arbitration latency under contention.
- Spinlock overhead (cache line bouncing effect, though TC397 lacks coherent caches).
- Throughput degradation from shared memory access.
- Performance isolation between cores (local DSPR work should be unaffected).

### 6.8 Summary: Test Effectiveness Matrix

| Test Pattern | Speed | Size | Power | Throughput | Multicore | Difficulty |
|---|---|---|---|---|---|---|
| Matrix Multiply | HIGH | MED | MED | HIGH | LOW | Easy |
| FFT | HIGH | MED | MED | HIGH | LOW | Medium |
| CRC (table vs bit) | HIGH | HIGH | LOW | MED | LOW | Easy |
| Sorting Algorithms | MED | MED | LOW | MED | LOW | Easy |
| Memory Copy/Fill | MED | LOW | MED | HIGH | MED | Easy |
| Interrupt Storm | HIGH | LOW | MED | MED | HIGH | Medium |
| Multicore Contention | HIGH | LOW | MED | HIGH | VERY HIGH | Hard |
| Flash vs DSPR exec | VERY HIGH | N/A | HIGH | VERY HIGH | LOW | Easy |
| Packed vs Scalar ops | MED | MED | MED | HIGH | LOW | Medium |
| Branch-heavy vs free | MED | MED | LOW | MED | LOW | Easy |
| LUT vs Computation | MED | HIGH | MED | MED | LOW | Easy |
| NOP vs Compute loop | LOW | LOW | HIGH | N/A | LOW | Easy |

### 6.9 Recommended Test Combinations for Maximum Differentiation

For creating two binaries with the **maximum measurable performance difference** across all
metrics simultaneously:

**Binary A ("Optimized")**:
- Critical code in PSPR, data in DSPR (section attributes).
- Compile with `-O2 -funroll-loops -finline-functions`.
- Use packed halfword operations for 16-bit data processing.
- Use LUT-based algorithms (CRC table, sin LUT).
- Tight loops with hardware loop opportunity.
- Sequential memory access patterns.
- DMA for bulk memory operations.

**Binary B ("Baseline/Worst-case")**:
- All code in PFlash (default), data in LMU or EMEM.
- Compile with `-O0` or `-Os -fno-inline`.
- Scalar-only operations.
- Bit-by-bit algorithms (CRC bit-by-bit, computation-based trig).
- Deeply nested function calls (no inlining).
- Random/strided memory access patterns.
- CPU-based memory operations.

Expected differences:
- **Execution speed**: 3-20x slower for Binary B.
- **Binary size**: Binary A 20-50% larger (due to inlining and LUTs).
- **Power consumption**: Binary B 10-30% higher power per operation (more flash accesses,
  more bus activity, longer execution time means more total energy).
- **Throughput**: Binary A 2-8x higher throughput depending on the workload.

---

## Appendix A: Key tricore-gcc Compiler Flags for Performance Testing

```
# Optimization levels
-O0          # No optimization (maximum debugging, slowest code)
-O1          # Basic optimization
-O2          # Full optimization (recommended for production)
-O3          # Aggressive optimization (may increase code size)
-Os          # Optimize for size (may sacrifice speed)

# Inlining control
-finline-functions           # Inline small functions
-fno-inline                  # Disable all inlining
-finline-limit=N             # Inline functions smaller than N pseudo-instructions

# Loop optimization
-funroll-loops               # Unroll loops with known iteration count
-funroll-all-loops           # Unroll all loops (aggressive)
-fno-unroll-loops            # Disable loop unrolling

# Code generation
-ffunction-sections          # Place each function in its own section
-fdata-sections              # Place each data item in its own section
# (Linker: --gc-sections to remove unused sections)

# TriCore-specific
-mtc161                      # Target TC1.6.1 (use for TC397)
-mtc162                      # Target TC1.6.2 (preferred for TC397)
-msmall-data=N               # Small data threshold
-msmall-const=N              # Small constant threshold

# Useful for analysis
-S                           # Generate assembly output
-fverbose-asm                # Annotate assembly with C source
-fdump-rtl-all               # Dump RTL passes for analysis
```

## Appendix B: Performance Counter Configuration (TC397)

The TC397 provides 5 performance counters per core:

| Counter | CSFR Address | What it counts |
|---|---|---|
| CCNT | CPU_CCNT | CPU clock cycles |
| ICNT | CPU_ICNT | Instructions retired |
| M1CNT | CPU_M1CNT | Configurable event 1 |
| M2CNT | CPU_M2CNT | Configurable event 2 |
| M3CNT | CPU_M3CNT | Configurable event 3 |

M1CNT, M2CNT, M3CNT can be configured to count various events including:
- Data cache misses
- Program cache misses
- Branch mispredictions
- Pipeline stalls
- Bus wait cycles

Usage pattern:
```c
IfxCpu_resetAndStartCounters(IfxCpu_CounterMode_normal);
/* ... code under test ... */
IfxCpu_Perf result = IfxCpu_stopCounters();

float ipc = (float)result.instruction.counter / (float)result.clock.counter;
/* IPC of ~1.0 is typical; >1.5 indicates good dual-issue; <0.5 indicates many stalls */
```

## Appendix C: Memory Placement Cheatsheet

```c
/* Place data in CPU0's DSPR (zero wait state for CPU0) */
uint32 my_data[100] __attribute__((section(".data_cpu0")));

/* Place code in CPU0's PSPR (zero wait state instruction fetch) */
__attribute__((section(".psram_cpu0")))
void my_fast_function(void) { /* ... */ }

/* Place data in LMU (shared, accessible by all cores) */
uint32 shared_data[100] __attribute__((section(".lmuram")));

/* Place data in non-cached LMU (for DMA or cross-core sharing) */
uint32 nc_data[100] __attribute__((section(".lmuram_nc")));

/* Place large constant tables in specific PFlash bank */
const uint32 big_table[4096] __attribute__((section(".rodata")));

/* Place data in EMEM (4MB, shared, higher latency) */
uint32 emem_data[100000] __attribute__((section(".edmem")));

/* Force alignment for efficient access */
uint32 aligned_buf[256] __attribute__((aligned(32))); /* Cache-line aligned */
```

Note: The exact section names must match those defined in the project's linker script
(`Lcf_Gnuc_Tricore_Tc.lsl`). Custom sections may need to be added to the linker script
to target specific memory regions.
