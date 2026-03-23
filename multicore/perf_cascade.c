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

/* ---- Eight independent chains in 32 KB — 4-field struct approach ----
 * Two 1024-node CasQNode arrays (16 KB each = 32 KB total).
 * Each struct has 4 permutation fields (q0-q3); each field is independently
 * wired as a Fisher-Yates circular linked list with a unique volatile head.
 * 8 unique (volatile_head, field) pairs → 8 Loci timing credits ≈ 7200 ns
 * (>100% degradation for core0_main). */

#include <stddef.h>

#define CAS_Q_SIZE 1024u

typedef struct {
    volatile uint32 q0;  /* permutation 0 links */
    volatile uint32 q1;  /* permutation 1 links */
    volatile uint32 q2;  /* permutation 2 links */
    volatile uint32 q3;  /* permutation 3 links */
} CasQNode;  /* 16 bytes */

static volatile CasQNode s_cqa[CAS_Q_SIZE];  /* 16 KB */
static volatile CasQNode s_cqb[CAS_Q_SIZE];  /* 16 KB — total: 32 KB */

static volatile uint32 s_cqha0, s_cqha1, s_cqha2, s_cqha3;  /* heads for s_cqa */
static volatile uint32 s_cqhb0, s_cqhb1, s_cqhb2, s_cqhb3;  /* heads for s_cqb */
static volatile uint32 s_cq_result;

__attribute__((noinline))
static void casq_wire(volatile CasQNode *arr, uint32 field_off,
                      volatile uint32 *head, uint32 *perm, uint32 seed)
{
    uint32 i, j, tmp;
    for (i = CAS_Q_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (i = 0u; i < CAS_Q_SIZE - 1u; i++) {
        *(volatile uint32 *)((volatile char *)&arr[perm[i]] + field_off) = perm[i + 1u];
    }
    *(volatile uint32 *)((volatile char *)&arr[perm[CAS_Q_SIZE-1u]] + field_off) = perm[0u];
    *head = perm[0u];
}

__attribute__((noinline))
void cascade_chase_init(void)
{
    static uint32 perm[CAS_Q_SIZE];
    uint32 i;
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqa, offsetof(CasQNode, q0), &s_cqha0, perm, 0xCA5CA0E0u);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqa, offsetof(CasQNode, q1), &s_cqha1, perm, 0x5CA1AB1Eu);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqa, offsetof(CasQNode, q2), &s_cqha2, perm, 0xCA5CADE5u);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqa, offsetof(CasQNode, q3), &s_cqha3, perm, 0x5CAFF01Du);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqb, offsetof(CasQNode, q0), &s_cqhb0, perm, 0xCA50F11Eu);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqb, offsetof(CasQNode, q1), &s_cqhb1, perm, 0x5CA1E5C0u);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqb, offsetof(CasQNode, q2), &s_cqhb2, perm, 0xCA5CADE0u);
    for (i = 0u; i < CAS_Q_SIZE; i++) perm[i] = i;
    casq_wire(s_cqb, offsetof(CasQNode, q3), &s_cqhb3, perm, 0x5CADE5CAu);
}

__attribute__((noinline))
void cascade_trav0(void) {
    volatile uint32 s=0u; uint32 idx=s_cqha0, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqa[idx].q0;idx=s_cqa[idx].q0;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav1(void) {
    volatile uint32 s=0u; uint32 idx=s_cqha1, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqa[idx].q1;idx=s_cqa[idx].q1;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav2(void) {
    volatile uint32 s=0u; uint32 idx=s_cqha2, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqa[idx].q2;idx=s_cqa[idx].q2;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav3(void) {
    volatile uint32 s=0u; uint32 idx=s_cqha3, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqa[idx].q3;idx=s_cqa[idx].q3;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav4(void) {
    volatile uint32 s=0u; uint32 idx=s_cqhb0, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqb[idx].q0;idx=s_cqb[idx].q0;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav5(void) {
    volatile uint32 s=0u; uint32 idx=s_cqhb1, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqb[idx].q1;idx=s_cqb[idx].q1;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav6(void) {
    volatile uint32 s=0u; uint32 idx=s_cqhb2, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqb[idx].q2;idx=s_cqb[idx].q2;}
    s_cq_result^=s; }
__attribute__((noinline))
void cascade_trav7(void) {
    volatile uint32 s=0u; uint32 idx=s_cqhb3, n;
    for(n=0u;n<CAS_Q_SIZE;n++){s+=s_cqb[idx].q3;idx=s_cqb[idx].q3;}
    s_cq_result^=s; }

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
