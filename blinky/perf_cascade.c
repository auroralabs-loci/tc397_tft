/* PERF-009: Six-Core In-Loop Cascade — Simultaneous Multi-Core Degradation
 * All 6 cores run heavy compute + memory workloads EVERY iteration of their
 * while(1) loops. A shared volatile counter creates crossbar bus contention.
 * 19 new GLOBAL symbols total: 12 workers + 1 sync + 6 orchestrators.
 * Designed to achieve >100% per-core response time degradation. */

#include "perf_cascade.h"

/* Shared volatile counter — ALL 6 cores read-modify-write this each iteration.
 * Forces a serialization point on the TC397 SRI crossbar. */
volatile uint32 cascade_shared_counter = 0u;

/* Per-core working arrays (volatile — prevent dead code elimination) */
static volatile sint32 s_cas_mat0[16][16];
static volatile sint32 s_cas_mat1[16][16];
static volatile sint32 s_cas_mat2[16][16];
static volatile sint32 s_cas_mat3[16][16];
static volatile sint32 s_cas_mat4[16][16];
static volatile sint32 s_cas_mat5[16][16];

static volatile uint32 s_cas_mem0[256];
static volatile uint32 s_cas_mem1[256];
static volatile uint32 s_cas_mem2[256];
static volatile uint32 s_cas_mem3[256];
static volatile uint32 s_cas_mem4[256];
static volatile uint32 s_cas_mem5[256];

/* ---- CORE 0 ---- */
__attribute__((noinline))
void cascade_compute_core0(void)
{
    sint32 i, j, k, acc;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            s_cas_mat0[i][j] = (sint32)(i * 17 + j + 1);
        }
    }
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            acc = 0;
            for (k = 0; k < 16; k++) {
                acc += s_cas_mat0[i][k] * s_cas_mat0[k][j];
            }
            s_cas_mat0[i][j] = acc;
        }
    }
}

__attribute__((noinline))
void cascade_memory_core0(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_cas_mem0[i] = s_cas_mem0[i] ^ (uint32)(i * 3 + 1);
    }
    s_cas_mem0[0] ^= s_cas_mat0[0][0];
}

/* ---- CORE 1 ---- */
__attribute__((noinline))
void cascade_compute_core1(void)
{
    sint32 i, j, k, acc;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            s_cas_mat1[i][j] = (sint32)(i * 13 + j + 2);
        }
    }
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            acc = 0;
            for (k = 0; k < 16; k++) {
                acc += s_cas_mat1[i][k] * s_cas_mat1[k][j];
            }
            s_cas_mat1[i][j] = acc;
        }
    }
}

__attribute__((noinline))
void cascade_memory_core1(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_cas_mem1[i] = s_cas_mem1[i] + (uint32)(i * 5 + 1);
    }
    s_cas_mem1[0] ^= s_cas_mat1[0][0];
}

/* ---- CORE 2 ---- */
__attribute__((noinline))
void cascade_compute_core2(void)
{
    sint32 i, j, k, acc;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            s_cas_mat2[i][j] = (sint32)(i * 19 + j + 3);
        }
    }
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            acc = 0;
            for (k = 0; k < 16; k++) {
                acc += s_cas_mat2[i][k] * s_cas_mat2[k][j];
            }
            s_cas_mat2[i][j] = acc;
        }
    }
}

__attribute__((noinline))
void cascade_memory_core2(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_cas_mem2[i] = s_cas_mem2[i] - (uint32)(i * 7 + 1);
    }
    s_cas_mem2[0] ^= s_cas_mat2[0][0];
}

/* ---- CORE 3 ---- */
__attribute__((noinline))
void cascade_compute_core3(void)
{
    sint32 i, j, k, acc;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            s_cas_mat3[i][j] = (sint32)(i * 23 + j + 4);
        }
    }
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            acc = 0;
            for (k = 0; k < 16; k++) {
                acc += s_cas_mat3[i][k] * s_cas_mat3[k][j];
            }
            s_cas_mat3[i][j] = acc;
        }
    }
}

__attribute__((noinline))
void cascade_memory_core3(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_cas_mem3[i] = s_cas_mem3[i] ^ (uint32)(i * 11 + 1);
    }
    s_cas_mem3[0] ^= s_cas_mat3[0][0];
}

/* ---- CORE 4 ---- */
__attribute__((noinline))
void cascade_compute_core4(void)
{
    sint32 i, j, k, acc;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            s_cas_mat4[i][j] = (sint32)(i * 29 + j + 5);
        }
    }
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            acc = 0;
            for (k = 0; k < 16; k++) {
                acc += s_cas_mat4[i][k] * s_cas_mat4[k][j];
            }
            s_cas_mat4[i][j] = acc;
        }
    }
}

__attribute__((noinline))
void cascade_memory_core4(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_cas_mem4[i] = s_cas_mem4[i] + (uint32)(i * 13 + 1);
    }
    s_cas_mem4[0] ^= s_cas_mat4[0][0];
}

/* ---- CORE 5 ---- */
__attribute__((noinline))
void cascade_compute_core5(void)
{
    sint32 i, j, k, acc;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            s_cas_mat5[i][j] = (sint32)(i * 31 + j + 6);
        }
    }
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            acc = 0;
            for (k = 0; k < 16; k++) {
                acc += s_cas_mat5[i][k] * s_cas_mat5[k][j];
            }
            s_cas_mat5[i][j] = acc;
        }
    }
}

__attribute__((noinline))
void cascade_memory_core5(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_cas_mem5[i] = s_cas_mem5[i] ^ (uint32)(i * 17 + 1);
    }
    s_cas_mem5[0] ^= s_cas_mat5[0][0];
}

/* Shared barrier — ALL cores call this. Forces serialization on crossbar.
 * Simple increment of shared volatile counter. */
__attribute__((noinline))
void cascade_sync_barrier(void)
{
    cascade_shared_counter += 1u;
}

/* ---- Eight independent volatile pointer-chase chains for core0 ----
 * cascade_chase_init() called before while(1); cascade_travN() per iteration.
 * 8 × ~900 ns ≈ 7200 ns (>100% degradation for core0_main). */

#define CAS_C_SIZE 512u

typedef struct {
    volatile uint32 next_idx;
    volatile uint32 value;
    volatile uint32 pad0;
    volatile uint32 pad1;
} CasChainNode;

static volatile CasChainNode s_ccn0[CAS_C_SIZE], s_ccn1[CAS_C_SIZE];
static volatile CasChainNode s_ccn2[CAS_C_SIZE], s_ccn3[CAS_C_SIZE];
static volatile CasChainNode s_ccn4[CAS_C_SIZE], s_ccn5[CAS_C_SIZE];
static volatile CasChainNode s_ccn6[CAS_C_SIZE], s_ccn7[CAS_C_SIZE];

static volatile uint32 s_cch0, s_cch1, s_cch2, s_cch3;
static volatile uint32 s_cch4, s_cch5, s_cch6, s_cch7;

__attribute__((noinline))
static void cas_init_one(volatile CasChainNode *nodes, volatile uint32 *head,
                         uint32 seed)
{
    uint32 perm[CAS_C_SIZE];
    uint32 i, j, tmp;
    for (i = 0u; i < CAS_C_SIZE; i++) { perm[i] = i; nodes[i].value = i + 1u; }
    for (i = CAS_C_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (i = 0u; i < CAS_C_SIZE - 1u; i++) { nodes[perm[i]].next_idx = perm[i + 1u]; }
    nodes[perm[CAS_C_SIZE - 1u]].next_idx = perm[0u];
    *head = perm[0u];
}

__attribute__((noinline))
void cascade_chase_init(void)
{
    cas_init_one(s_ccn0, &s_cch0, 0xCA5CA0E0u);
    cas_init_one(s_ccn1, &s_cch1, 0x5CA1AB1Eu);
    cas_init_one(s_ccn2, &s_cch2, 0xCA5CADE5u);
    cas_init_one(s_ccn3, &s_cch3, 0x5CAFF01Du);
    cas_init_one(s_ccn4, &s_cch4, 0xCA50F11Eu);
    cas_init_one(s_ccn5, &s_cch5, 0x5CA1E5C0u);
    cas_init_one(s_ccn6, &s_cch6, 0xCA5CADE0u);
    cas_init_one(s_ccn7, &s_cch7, 0x5CADE5CAu);
}

__attribute__((noinline))
void cascade_trav0(void) {
    volatile uint32 s=0u; uint32 idx=s_cch0, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn0[idx].value;idx=s_ccn0[idx].next_idx;}
    s_ccn0[s_cch0].pad0^=s; }
__attribute__((noinline))
void cascade_trav1(void) {
    volatile uint32 s=0u; uint32 idx=s_cch1, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn1[idx].value;idx=s_ccn1[idx].next_idx;}
    s_ccn1[s_cch1].pad0^=s; }
__attribute__((noinline))
void cascade_trav2(void) {
    volatile uint32 s=0u; uint32 idx=s_cch2, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn2[idx].value;idx=s_ccn2[idx].next_idx;}
    s_ccn2[s_cch2].pad0^=s; }
__attribute__((noinline))
void cascade_trav3(void) {
    volatile uint32 s=0u; uint32 idx=s_cch3, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn3[idx].value;idx=s_ccn3[idx].next_idx;}
    s_ccn3[s_cch3].pad0^=s; }
__attribute__((noinline))
void cascade_trav4(void) {
    volatile uint32 s=0u; uint32 idx=s_cch4, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn4[idx].value;idx=s_ccn4[idx].next_idx;}
    s_ccn4[s_cch4].pad0^=s; }
__attribute__((noinline))
void cascade_trav5(void) {
    volatile uint32 s=0u; uint32 idx=s_cch5, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn5[idx].value;idx=s_ccn5[idx].next_idx;}
    s_ccn5[s_cch5].pad0^=s; }
__attribute__((noinline))
void cascade_trav6(void) {
    volatile uint32 s=0u; uint32 idx=s_cch6, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn6[idx].value;idx=s_ccn6[idx].next_idx;}
    s_ccn6[s_cch6].pad0^=s; }
__attribute__((noinline))
void cascade_trav7(void) {
    volatile uint32 s=0u; uint32 idx=s_cch7, n;
    for(n=0u;n<CAS_C_SIZE;n++){s+=s_ccn7[idx].value;idx=s_ccn7[idx].next_idx;}
    s_ccn7[s_cch7].pad0^=s; }

/* ---- Per-core orchestrators ---- */
__attribute__((noinline))
void cascade_run_core0(void)
{
    cascade_compute_core0();
    cascade_memory_core0();
    cascade_sync_barrier();
    /* Eight independent volatile-chase traversals → >100% core0 degradation */
    cascade_trav0(); cascade_trav1(); cascade_trav2(); cascade_trav3();
    cascade_trav4(); cascade_trav5(); cascade_trav6(); cascade_trav7();
}

__attribute__((noinline))
void cascade_run_core1(void)
{
    cascade_compute_core1();
    cascade_memory_core1();
    cascade_sync_barrier();
}

__attribute__((noinline))
void cascade_run_core2(void)
{
    cascade_compute_core2();
    cascade_memory_core2();
    cascade_sync_barrier();
}

__attribute__((noinline))
void cascade_run_core3(void)
{
    cascade_compute_core3();
    cascade_memory_core3();
    cascade_sync_barrier();
}

__attribute__((noinline))
void cascade_run_core4(void)
{
    cascade_compute_core4();
    cascade_memory_core4();
    cascade_sync_barrier();
}

__attribute__((noinline))
void cascade_run_core5(void)
{
    cascade_compute_core5();
    cascade_memory_core5();
    cascade_sync_barrier();
}
