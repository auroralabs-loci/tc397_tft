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

/* ---- Eight independent volatile pointer-chase chains ----
 * 8 × 512 nodes × 16 B = 64 KB volatile BSS.  Each chain has its own
 * volatile array and volatile head so Loci credits each traverse
 * independently: 8 × ~900 ns ≈ 7200 ns (>100% degradation target).
 * bloat_chains_init() wires all eight before while(1). */

#define BC_SIZE 512u

typedef struct {
    volatile uint32 next_idx;
    volatile uint32 value;
    volatile uint32 pad0;
    volatile uint32 pad1;
} BloatChainNode; /* 16 bytes */

static volatile BloatChainNode s_bcn0[BC_SIZE], s_bcn1[BC_SIZE];
static volatile BloatChainNode s_bcn2[BC_SIZE], s_bcn3[BC_SIZE];
static volatile BloatChainNode s_bcn4[BC_SIZE], s_bcn5[BC_SIZE];
static volatile BloatChainNode s_bcn6[BC_SIZE], s_bcn7[BC_SIZE];

static volatile uint32 s_bh0, s_bh1, s_bh2, s_bh3;
static volatile uint32 s_bh4, s_bh5, s_bh6, s_bh7;

/* Per-chain Fisher-Yates init — noinline so Loci treats the resulting
 * volatile node data as UNKNOWN when analysing the traverse functions. */
__attribute__((noinline))
static void bc_init_one(volatile BloatChainNode *nodes, volatile uint32 *head,
                        uint32 seed)
{
    uint32 perm[BC_SIZE]; /* 2 KB stack — acceptable at 512 entries */
    uint32 i, j, tmp;
    for (i = 0u; i < BC_SIZE; i++) { perm[i] = i; nodes[i].value = i + 1u; }
    for (i = BC_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (i = 0u; i < BC_SIZE - 1u; i++) { nodes[perm[i]].next_idx = perm[i + 1u]; }
    nodes[perm[BC_SIZE - 1u]].next_idx = perm[0u];
    *head = perm[0u];
}

/* Initialise all 8 chains once before while(1). */
__attribute__((noinline))
void bloat_chains_init(void)
{
    bc_init_one(s_bcn0, &s_bh0, 0xABCDEF01u);
    bc_init_one(s_bcn1, &s_bh1, 0x12345678u);
    bc_init_one(s_bcn2, &s_bh2, 0xDEADBEEFu);
    bc_init_one(s_bcn3, &s_bh3, 0xCAFEBABEu);
    bc_init_one(s_bcn4, &s_bh4, 0xFEEDFACEu);
    bc_init_one(s_bcn5, &s_bh5, 0xC0FFEE00u);
    bc_init_one(s_bcn6, &s_bh6, 0x0BADF00Du);
    bc_init_one(s_bcn7, &s_bh7, 0x8BADF00Du);
}

/* Eight independent traverse functions — each reads its OWN volatile head
 * and follows its OWN volatile array, giving Loci 8 independent timing credits. */
__attribute__((noinline))
void bloat_trav0(void) {
    volatile uint32 s=0u; uint32 idx=s_bh0, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn0[idx].value;idx=s_bcn0[idx].next_idx;}
    s_bcn0[s_bh0].pad0^=s; }
__attribute__((noinline))
void bloat_trav1(void) {
    volatile uint32 s=0u; uint32 idx=s_bh1, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn1[idx].value;idx=s_bcn1[idx].next_idx;}
    s_bcn1[s_bh1].pad0^=s; }
__attribute__((noinline))
void bloat_trav2(void) {
    volatile uint32 s=0u; uint32 idx=s_bh2, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn2[idx].value;idx=s_bcn2[idx].next_idx;}
    s_bcn2[s_bh2].pad0^=s; }
__attribute__((noinline))
void bloat_trav3(void) {
    volatile uint32 s=0u; uint32 idx=s_bh3, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn3[idx].value;idx=s_bcn3[idx].next_idx;}
    s_bcn3[s_bh3].pad0^=s; }
__attribute__((noinline))
void bloat_trav4(void) {
    volatile uint32 s=0u; uint32 idx=s_bh4, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn4[idx].value;idx=s_bcn4[idx].next_idx;}
    s_bcn4[s_bh4].pad0^=s; }
__attribute__((noinline))
void bloat_trav5(void) {
    volatile uint32 s=0u; uint32 idx=s_bh5, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn5[idx].value;idx=s_bcn5[idx].next_idx;}
    s_bcn5[s_bh5].pad0^=s; }
__attribute__((noinline))
void bloat_trav6(void) {
    volatile uint32 s=0u; uint32 idx=s_bh6, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn6[idx].value;idx=s_bcn6[idx].next_idx;}
    s_bcn6[s_bh6].pad0^=s; }
__attribute__((noinline))
void bloat_trav7(void) {
    volatile uint32 s=0u; uint32 idx=s_bh7, n;
    for(n=0u;n<BC_SIZE;n++){s+=s_bcn7[idx].value;idx=s_bcn7[idx].next_idx;}
    s_bcn7[s_bh7].pad0^=s; }

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
