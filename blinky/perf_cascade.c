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

/* ---- Per-core orchestrators ---- */
__attribute__((noinline))
void cascade_run_core0(void)
{
    cascade_compute_core0();
    cascade_memory_core0();
    cascade_sync_barrier();
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
