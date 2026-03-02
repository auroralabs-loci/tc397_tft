/* PERF-008: Branch Prediction Destroyer — Data-Dependent Pipeline Stalls
 * All functions use data-dependent conditionals so the branch predictor
 * cannot build a useful table. Re-scramble after each use ensures every
 * iteration presents a novel branch sequence.
 * Designed to achieve >100% response time degradation. */

#include "perf_branch.h"

/* Working buffers — volatile prevents dead-code elimination */
static volatile sint32 s_br_sort_buf[512];
static volatile sint32 s_br_thresh_buf[512];
static volatile uint32 s_br_search_buf[512];
static volatile uint32 s_br_result;   /* prevent optimisation of entire calls */

/* LCG state — updated each call to produce different branch sequences */
static volatile uint32 s_br_lcg_seed = 0xDEADBEEFu;

/* Advance the LCG and return next value */
static uint32 br_lcg_next(void)
{
    s_br_lcg_seed = s_br_lcg_seed * 1664525u + 1013904223u;
    return s_br_lcg_seed;
}

/* Selection sort on 512 volatile elements.
 * Re-scrambles the array with the LCG after each sort to ensure the
 * branch sequence changes every iteration. O(n^2) comparisons. */
__attribute__((noinline))
void branch_data_dependent_sort(void)
{
    sint32 i, j, min_idx;
    sint32 tmp;
    /* Re-scramble: fill with LCG values so sequence changes each iteration */
    for (i = 0; i < 512; i++) {
        s_br_sort_buf[i] = (sint32)br_lcg_next();
    }
    /* Selection sort — O(n^2) comparisons, all data-dependent branches */
    for (i = 0; i < 512 - 1; i++) {
        min_idx = i;
        for (j = i + 1; j < 512; j++) {
            if (s_br_sort_buf[j] < s_br_sort_buf[min_idx]) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            tmp = s_br_sort_buf[i];
            s_br_sort_buf[i] = s_br_sort_buf[min_idx];
            s_br_sort_buf[min_idx] = tmp;
        }
    }
    s_br_result = (uint32)s_br_sort_buf[0];
}

/* Each of 512 elements is checked against 4 thresholds in a nested
 * if-else chain. Thresholds derived from previous element outcome
 * create a dependency chain that defeats speculative execution. */
__attribute__((noinline))
void branch_threshold_cascade(void)
{
    sint32 i;
    sint32 val;
    volatile sint32 acc = 0;
    volatile sint32 th1 = (sint32)(br_lcg_next() & 0x7FFFFFFFu) / 4;
    volatile sint32 th2 = th1 / 2;
    volatile sint32 th3 = th2 / 2;
    volatile sint32 th4 = th3 / 2;

    for (i = 0; i < 512; i++) {
        val = s_br_thresh_buf[i] = (sint32)br_lcg_next();
        if (val > th1) {
            acc += val;
            th1 = acc / 4;  /* threshold changes: dependency chain */
        } else if (val > th2) {
            acc += val / 2;
            th2 = acc / 4;
        } else if (val > th3) {
            acc -= val / 4;
            th3 = acc / 4;
        } else if (val > th4) {
            acc -= val;
            th4 = acc / 4;
        } else {
            acc ^= val;
        }
    }
    s_br_result ^= (uint32)acc;
}

/* 512 iterations; each iteration tests 8 bits of a 32-bit LFSR word.
 * 8 independent conditional branches per iteration = 4096 branches total.
 * LFSR advances each iteration so bit patterns are always different. */
__attribute__((noinline))
void branch_bit_scatter(void)
{
    uint32 lfsr = (uint32)s_br_result ^ 0xACE1u;
    volatile uint32 acc = 0u;
    sint32 i;
    uint32 bit;

    if (lfsr == 0u) { lfsr = 1u; }  /* avoid zero state */

    for (i = 0; i < 512; i++) {
        /* Galois LFSR step */
        bit  = lfsr & 1u;
        lfsr >>= 1u;
        if (bit) { lfsr ^= 0xB400u; }

        /* 8 data-dependent branches on separate bit positions */
        if (lfsr & 0x01u)   { acc += 1u; }
        if (lfsr & 0x02u)   { acc += 2u; }
        if (lfsr & 0x04u)   { acc += 4u; }
        if (lfsr & 0x08u)   { acc += 8u; }
        if (lfsr & 0x10u)   { acc += 16u; }
        if (lfsr & 0x20u)   { acc += 32u; }
        if (lfsr & 0x40u)   { acc += 64u; }
        if (lfsr & 0x80u)   { acc += 128u; }
    }
    s_br_result ^= acc;
}

/* Linear search through 512 volatile elements for a target that changes
 * every call. Taken/not-taken ratio is ~1:511, maximally skewed. */
__attribute__((noinline))
void branch_search_unsorted(void)
{
    uint32 target;
    sint32 i;
    volatile sint32 found_at = -1;

    /* Fill buffer with LCG values */
    for (i = 0; i < 512; i++) {
        s_br_search_buf[i] = br_lcg_next();
    }
    /* Target is one specific LCG value — changes every call */
    target = br_lcg_next();

    for (i = 0; i < 512; i++) {
        if (s_br_search_buf[i] == target) {
            found_at = i;
            break;
        }
    }
    s_br_result ^= (uint32)(found_at + 1);
}

/* 512 iterations, each with 3 conditions that individually trigger often
 * but never all three simultaneously — so the "early exit" never fires.
 * Predictor learns each condition in isolation but cannot predict the
 * conjunction, causing repeated mispredictions on the AND evaluation. */
__attribute__((noinline))
void branch_early_exit_sabotage(void)
{
    sint32 i;
    volatile uint32 acc = 0u;
    uint32 v;

    for (i = 0; i < 512; i++) {
        v = br_lcg_next();
        /* Each condition is true ~50% of the time independently.
         * All three simultaneously: ~12.5% — never used as exit condition.
         * The branch predictor cannot accurately predict the combined test. */
        if ((v & 1u) && (v & 4u) && (v & 16u)) {
            /* This path taken ~12.5% — not an early exit, just expensive work */
            acc += v * v;
        } else {
            acc ^= v;
        }
        /* Additional unpredictable branch in else path */
        if ((v & 0xAAu) == 0xAAu) {
            acc += i;
        }
    }
    s_br_result ^= acc;
}

/* 4-level deep dispatch tree, 8 cases each level.
 * Each level's selector comes from the previous level's result.
 * ~4096 distinct paths; distribution is LCG-seeded so uniform. */
__attribute__((noinline))
void branch_nested_dispatch(void)
{
    uint32 v = br_lcg_next();
    volatile uint32 acc = 0u;
    sint32 rep;

    for (rep = 0; rep < 64; rep++) {
        v = br_lcg_next();
        switch (v & 7u) {           /* level 1 */
        case 0u: acc += v;          break;
        case 1u: acc ^= v;          break;
        case 2u: acc -= v;          break;
        case 3u: acc += v >> 1;     break;
        case 4u: acc ^= v << 1;     break;
        case 5u: acc += v & 0xFFu;  break;
        case 6u: acc -= v & 0xFFu;  break;
        default: acc ^= v >> 2;     break;
        }
        switch ((acc ^ v) & 7u) {   /* level 2 — selector depends on level 1 */
        case 0u: acc += v * 3u;     break;
        case 1u: acc ^= v * 5u;     break;
        case 2u: acc += v >> 2;     break;
        case 3u: acc -= v >> 2;     break;
        case 4u: acc ^= v * 7u;     break;
        case 5u: acc += v & 0xF0u;  break;
        case 6u: acc -= v & 0x0Fu;  break;
        default: acc ^= v;          break;
        }
        switch ((acc + v) & 7u) {   /* level 3 */
        case 0u: acc |= v;          break;
        case 1u: acc &= v | 1u;     break;
        case 2u: acc += v ^ acc;    break;
        case 3u: acc ^= v + acc;    break;
        case 4u: acc -= v ^ acc;    break;
        case 5u: acc |= v >> 3;     break;
        case 6u: acc &= ~v;         break;
        default: acc += v & acc;    break;
        }
        switch ((acc ^ (v >> 1)) & 7u) { /* level 4 */
        case 0u: acc += v >> 3;     break;
        case 1u: acc ^= v << 2;     break;
        case 2u: acc -= v >> 3;     break;
        case 3u: acc += v;          break;
        case 4u: acc ^= v;          break;
        case 5u: acc -= v;          break;
        case 6u: acc += v & 0xFFFFu;break;
        default: acc ^= v & 0xFFFFu;break;
        }
    }
    s_br_result ^= acc;
}

/* ---- Volatile-index chase for load-use latency ---- */
static volatile uint32 s_br_chase_buf[256];
static volatile uint32 s_br_chase_result;

/* 10240-hop data-dependent load-use chain using a full-period LCG permutation
 * (a=197,c=13,m=256 satisfies Hull-Dobell → single cycle of length 256).
 * Complements the branch-pattern workloads by adding guaranteed serialised
 * memory latency, targeting >100% response time degradation. */
__attribute__((noinline))
void branch_volatile_chase(void)
{
    uint32 i;
    volatile uint32 cur;
    for (i = 0u; i < 256u; i++) {
        s_br_chase_buf[i] = (uint32)((i * 197u + 13u) & 0xFFu);
    }
    cur = s_br_chase_buf[0];
    for (i = 0u; i < 10240u; i++) {
        cur = s_br_chase_buf[cur & 255u];
    }
    s_br_chase_result ^= cur;
}

/* Orchestrator — called every while(1) iteration. */
__attribute__((noinline))
void branch_run_all(void)
{
    branch_data_dependent_sort();
    branch_threshold_cascade();
    branch_bit_scatter();
    branch_search_unsorted();
    branch_early_exit_sabotage();
    branch_nested_dispatch();
    branch_volatile_chase();   /* load-use chain: ensures >100% response time degradation */
}
