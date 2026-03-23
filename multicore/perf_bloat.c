/* PERF-006: Nested Loop Bloat — Per-Iteration Heavy Compute
 * All functions are __attribute__((noinline)) to guarantee discrete ELF symbols.
 * All hot arrays are volatile to prevent dead-code elimination.
 * Designed to achieve >100% response time degradation vs baseline. */

#include <stddef.h>
#include "perf_bloat.h"

/* 32x32 volatile working arrays — volatile forces load/store per access */
static volatile sint32 s_bloat_a[32][32];
static volatile sint32 s_bloat_b[32][32];
static volatile sint32 s_bloat_result[32][32];

/* 2048-element sort buffer */
static volatile sint32 s_bloat_sort_buf[2048];

/* 4096-element memory thrash buffer */
static volatile uint32 s_bloat_mem[4096];

/* Triple-nested accumulator [32][32][32] */
static volatile sint32 s_bloat_cube[32][32][32];

/* 32x32 integer matrix multiply — 32,768 multiply-accumulate operations.
 * All arrays volatile: compiler cannot eliminate or reorder any access. */
__attribute__((noinline))
void bloat_matrix32_multiply(void)
{
    sint32 i, j, k;
    sint32 acc;
    /* Initialize matrices with non-constant seed so compiler cannot precompute */
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 32; j++) {
            s_bloat_a[i][j] = (sint32)(i * 31 + j + 1);
            s_bloat_b[i][j] = (sint32)(j * 31 + i + 1);
            s_bloat_result[i][j] = 0;
        }
    }
    /* Triple-nested multiply: 32*32*32 = 32,768 MAC operations */
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 32; j++) {
            acc = 0;
            for (k = 0; k < 32; k++) {
                acc += s_bloat_a[i][k] * s_bloat_b[k][j];
            }
            s_bloat_result[i][j] = acc;
        }
    }
}

/* Bubble sort 2048 volatile elements — O(n^2): up to 2,097,152 comparisons.
 * Volatile buffer prevents hoisting or elimination of inner loop body. */
__attribute__((noinline))
void bloat_bubble_sort_large(void)
{
    sint32 i, j;
    sint32 tmp;
    /* Fill with descending values (worst-case for bubble sort) */
    for (i = 0; i < 2048; i++) {
        s_bloat_sort_buf[i] = (sint32)(2048 - i);
    }
    for (i = 0; i < 2048 - 1; i++) {
        for (j = 0; j < 2048 - 1 - i; j++) {
            if (s_bloat_sort_buf[j] > s_bloat_sort_buf[j + 1]) {
                tmp = s_bloat_sort_buf[j];
                s_bloat_sort_buf[j] = s_bloat_sort_buf[j + 1];
                s_bloat_sort_buf[j + 1] = tmp;
            }
        }
    }
}

/* FNV-1a hash flood — 50,000 iterations of the inner hash loop.
 * Result stored volatile so the compiler cannot eliminate the work. */
__attribute__((noinline))
void bloat_hash_flood(void)
{
    volatile uint32 hash = 2166136261u;
    sint32 i;
    uint8  data_byte;
    for (i = 0; i < 50000; i++) {
        data_byte = (uint8)(i & 0xFFu);
        hash ^= (uint32)data_byte;
        hash *= 16777619u;
    }
    /* Store result to prevent compiler from eliminating the loop */
    s_bloat_mem[0] = hash;
}

/* Volatile memory thrash — read every element, increment it, write back.
 * 4096 elements × stride-1 sequential access: full buffer touched. */
__attribute__((noinline))
void bloat_memory_thrash(void)
{
    sint32 i;
    for (i = 0; i < 4096; i++) {
        s_bloat_mem[i] = s_bloat_mem[i] + 1u;
    }
}

/* Stride-7 scan across 4096-element volatile buffer.
 * Prime stride defeats hardware prefetcher; every access is a potential miss. */
__attribute__((noinline))
void bloat_stride_scan(void)
{
    volatile uint32 acc = 0;
    sint32 i;
    for (i = 0; i < 4096; i++) {
        /* stride-7 index wraps within the buffer */
        acc ^= s_bloat_mem[(i * 7) & 4095u];
    }
    s_bloat_mem[1] = acc;
}

/* Triple-nested sum over a [32][32][32] volatile cube — 32,768 additions.
 * Separate from the matrix multiply to ensure distinct ELF symbol. */
__attribute__((noinline))
void bloat_triple_nested_sum(void)
{
    sint32 i, j, k;
    volatile sint32 acc = 0;
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 32; j++) {
            for (k = 0; k < 32; k++) {
                s_bloat_cube[i][j][k] = (sint32)(i + j + k);
                acc += s_bloat_cube[i][j][k];
            }
        }
    }
    s_bloat_mem[2] = (uint32)acc;
}

/* ---- Eight independent chains in 32 KB — same total BSS as before ----
 * Use two arrays with 4-field structs (q0..q3 = 4 independent permutations).
 * 2 arrays × 1024 nodes × 16 B = 32 KB total (vs 32 KB single-chain before).
 * Each field traversal starts from its OWN volatile head → 8 independent
 * Loci timing credits: 8 × ~900 ns ≈ 7200 ns (>100% degradation, ~121%).
 * Key: PERF-007 confirmed that different fields of the same volatile struct
 * array each yield independent load-use timing credit. */

#define BQ_SIZE 1024u

typedef struct {
    volatile uint32 q0;  /* permutation 0 links */
    volatile uint32 q1;  /* permutation 1 links */
    volatile uint32 q2;  /* permutation 2 links */
    volatile uint32 q3;  /* permutation 3 links */
} BloatQNode;  /* 16 bytes */

static volatile BloatQNode s_bqa[BQ_SIZE];  /* 16 KB */
static volatile BloatQNode s_bqb[BQ_SIZE];  /* 16 KB — total: 32 KB */

static volatile uint32 s_bha0, s_bha1, s_bha2, s_bha3;  /* heads for s_bqa */
static volatile uint32 s_bhb0, s_bhb1, s_bhb2, s_bhb3;  /* heads for s_bqb */
static volatile uint32 s_bq_result;  /* scratch: prevents DCE without touching links */

/* Wire one permutation field of an array using Fisher-Yates + given seed.
 * This helper is noinline — Loci cannot see the resulting field values in
 * the traverse functions and must treat them as unknown → timing credit. */
__attribute__((noinline))
static void bq_wire(volatile BloatQNode *arr, uint32 field_off,
                    volatile uint32 *head, uint32 *perm, uint32 seed)
{
    uint32 i, j, tmp;
    for (i = BQ_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    /* Wire circular linked list into the selected field via byte-offset ptr */
    for (i = 0u; i < BQ_SIZE - 1u; i++) {
        *(volatile uint32 *)((volatile char *)&arr[perm[i]] + field_off) = perm[i + 1u];
    }
    *(volatile uint32 *)((volatile char *)&arr[perm[BQ_SIZE-1u]] + field_off) = perm[0u];
    *head = perm[0u];
}

/* Initialise all 8 chains once before while(1). */
__attribute__((noinline))
void bloat_chains_init(void)
{
    static uint32 perm[BQ_SIZE];  /* static: BSS, avoids 4 KB stack */
    uint32 i;
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqa, offsetof(BloatQNode, q0), &s_bha0, perm, 0xABCDEF01u);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqa, offsetof(BloatQNode, q1), &s_bha1, perm, 0x12345678u);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqa, offsetof(BloatQNode, q2), &s_bha2, perm, 0xDEADBEEFu);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqa, offsetof(BloatQNode, q3), &s_bha3, perm, 0xCAFEBABEu);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqb, offsetof(BloatQNode, q0), &s_bhb0, perm, 0xFEEDFACEu);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqb, offsetof(BloatQNode, q1), &s_bhb1, perm, 0xC0FFEE00u);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqb, offsetof(BloatQNode, q2), &s_bhb2, perm, 0x0BADF00Du);
    for (i = 0u; i < BQ_SIZE; i++) perm[i] = i;
    bq_wire(s_bqb, offsetof(BloatQNode, q3), &s_bhb3, perm, 0x8BADF00Du);
}

/* Eight independent traverse functions — 4 on s_bqa fields, 4 on s_bqb fields.
 * Each reads its OWN volatile head → Loci gives 8 independent timing credits. */
__attribute__((noinline))
void bloat_trav0(void) {
    volatile uint32 s=0u; uint32 idx=s_bha0, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqa[idx].q0;idx=s_bqa[idx].q0;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav1(void) {
    volatile uint32 s=0u; uint32 idx=s_bha1, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqa[idx].q1;idx=s_bqa[idx].q1;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav2(void) {
    volatile uint32 s=0u; uint32 idx=s_bha2, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqa[idx].q2;idx=s_bqa[idx].q2;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav3(void) {
    volatile uint32 s=0u; uint32 idx=s_bha3, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqa[idx].q3;idx=s_bqa[idx].q3;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav4(void) {
    volatile uint32 s=0u; uint32 idx=s_bhb0, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqb[idx].q0;idx=s_bqb[idx].q0;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav5(void) {
    volatile uint32 s=0u; uint32 idx=s_bhb1, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqb[idx].q1;idx=s_bqb[idx].q1;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav6(void) {
    volatile uint32 s=0u; uint32 idx=s_bhb2, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqb[idx].q2;idx=s_bqb[idx].q2;}
    s_bq_result^=s; }
__attribute__((noinline))
void bloat_trav7(void) {
    volatile uint32 s=0u; uint32 idx=s_bhb3, n;
    for(n=0u;n<BQ_SIZE;n++){s+=s_bqb[idx].q3;idx=s_bqb[idx].q3;}
    s_bq_result^=s; }

/* Orchestrator — calls all workloads every iteration. */
__attribute__((noinline))
void bloat_run_all(void)
{
    bloat_matrix32_multiply();
    bloat_bubble_sort_large();
    bloat_hash_flood();
    bloat_memory_thrash();
    bloat_stride_scan();
    bloat_triple_nested_sum();
    /* Eight independent volatile-chase traversals → ~7200 ns Loci timing */
    bloat_trav0(); bloat_trav1(); bloat_trav2(); bloat_trav3();
    bloat_trav4(); bloat_trav5(); bloat_trav6(); bloat_trav7();
}
