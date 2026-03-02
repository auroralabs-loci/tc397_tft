/* PERF-006: Nested Loop Bloat — Per-Iteration Heavy Compute
 * All functions are __attribute__((noinline)) to guarantee discrete ELF symbols.
 * All hot arrays are volatile to prevent dead-code elimination.
 * Designed to achieve >100% response time degradation vs baseline. */

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

/* ---- Volatile pointer-chase linked list (PERF-007 architecture) ---- */
/* 2048 nodes × 16 bytes = 32 KB volatile global state.
 * PERF-007 showed 512 nodes (8 KB) → +1659 ns (+28%).
 * At 4× the volatile data this targets +6636 ns (112% degradation).
 * Critical: init is a SEPARATE function called before while(1) so that
 * the node content is UNKNOWN to Loci when it analyses bloat_chase_traverse. */
#define BLOAT_CHASE_SIZE 2048u

typedef struct {
    volatile uint32 next_idx;
    volatile uint32 value;
    volatile uint32 pad0;
    volatile uint32 pad1;
} BloatChaseNode;

static volatile BloatChaseNode s_bloat_chase_nodes[BLOAT_CHASE_SIZE];
static volatile uint32         s_bloat_chase_head;

/* Fisher-Yates permutation → circular singly-linked list.
 * Called ONCE before while(1) in Cpu0_Main.c. */
__attribute__((noinline))
void bloat_chase_init(void)
{
    static uint32 perm[BLOAT_CHASE_SIZE]; /* static: BSS, not stack */
    uint32 i, j, tmp, seed;
    for (i = 0u; i < BLOAT_CHASE_SIZE; i++) {
        perm[i] = i;
        s_bloat_chase_nodes[i].value = i + 1u;
    }
    seed = 0xABCDEF01u;
    for (i = BLOAT_CHASE_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (i = 0u; i < BLOAT_CHASE_SIZE - 1u; i++) {
        s_bloat_chase_nodes[perm[i]].next_idx = perm[i + 1u];
    }
    s_bloat_chase_nodes[perm[BLOAT_CHASE_SIZE - 1u]].next_idx = perm[0u];
    s_bloat_chase_head = perm[0u];
}

/* Traverse all 2048 nodes once via random-order pointer chain.
 * Called every while(1) iteration via bloat_run_all(). */
__attribute__((noinline))
void bloat_chase_traverse(void)
{
    volatile uint32 sum = 0u;
    uint32 idx = s_bloat_chase_head;
    uint32 step;
    for (step = 0u; step < BLOAT_CHASE_SIZE; step++) {
        sum += s_bloat_chase_nodes[idx].value;
        idx  = s_bloat_chase_nodes[idx].next_idx;
    }
    s_bloat_chase_nodes[s_bloat_chase_head].pad0 ^= sum;
}

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
    bloat_chase_traverse();    /* pointer-chase: cross-function volatile data → Loci timing */
}
