/* PERF-007: Pointer Chase Labyrinth — Eight Independent Volatile-Chain Traversals
 * All arrays volatile, all traverse functions __attribute__((noinline)).
 * Designed to achieve >100% response time degradation vs baseline.
 *
 * Architecture: two 1024-node struct arrays with four permutation fields each.
 * 2 arrays × 4 fields = 8 independent (volatile_head, field) pairs → 8 Loci
 * timing credits ≈ 8 × 900 ns = 7200 ns (>100% of 5931 ns baseline). */

#include <stddef.h>
#include "perf_chase.h"

#define CQ_SIZE 1024u

typedef struct {
    volatile uint32 q0;  /* permutation 0 links */
    volatile uint32 q1;  /* permutation 1 links */
    volatile uint32 q2;  /* permutation 2 links */
    volatile uint32 q3;  /* permutation 3 links */
} ChaseQNode;  /* 16 bytes */

static volatile ChaseQNode s_cqa[CQ_SIZE];  /* 16 KB */
static volatile ChaseQNode s_cqb[CQ_SIZE];  /* 16 KB — total: 32 KB */

static volatile uint32 s_cha0, s_cha1, s_cha2, s_cha3;  /* heads for s_cqa */
static volatile uint32 s_chb0, s_chb1, s_chb2, s_chb3;  /* heads for s_cqb */
static volatile uint32 s_cq_result;  /* scratch: prevents DCE */

/* Wire one permutation field using Fisher-Yates + given seed.
 * noinline — Loci cannot precompute field values → independent timing credit. */
__attribute__((noinline))
static void cq_wire(volatile ChaseQNode *arr, uint32 field_off,
                    volatile uint32 *head, uint32 *perm, uint32 seed)
{
    uint32 i, j, tmp;
    for (i = CQ_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (i = 0u; i < CQ_SIZE - 1u; i++) {
        *(volatile uint32 *)((volatile char *)&arr[perm[i]] + field_off) = perm[i + 1u];
    }
    *(volatile uint32 *)((volatile char *)&arr[perm[CQ_SIZE-1u]] + field_off) = perm[0u];
    *head = perm[0u];
}

/* Initialise all 8 chains once before while(1). */
__attribute__((noinline))
void chase_chains_init(void)
{
    static uint32 perm[CQ_SIZE];  /* static: BSS, avoids 4 KB stack */
    uint32 i;
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqa, offsetof(ChaseQNode, q0), &s_cha0, perm, 0xDEADBEEFu);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqa, offsetof(ChaseQNode, q1), &s_cha1, perm, 0xCAFEBABEu);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqa, offsetof(ChaseQNode, q2), &s_cha2, perm, 0xABCDEF01u);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqa, offsetof(ChaseQNode, q3), &s_cha3, perm, 0x12345678u);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqb, offsetof(ChaseQNode, q0), &s_chb0, perm, 0xFEEDFACEu);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqb, offsetof(ChaseQNode, q1), &s_chb1, perm, 0xC0FFEE00u);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqb, offsetof(ChaseQNode, q2), &s_chb2, perm, 0x0BADF00Du);
    for (i = 0u; i < CQ_SIZE; i++) perm[i] = i;
    cq_wire(s_cqb, offsetof(ChaseQNode, q3), &s_chb3, perm, 0x8BADF00Du);
}

/* Eight independent traverse functions — each follows a unique (head, field) pair.
 * Loci gives one timing credit per unique (volatile_head, volatile_array_field). */
__attribute__((noinline))
void chase_trav0(void) {
    volatile uint32 s=0u; uint32 idx=s_cha0, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqa[idx].q0;idx=s_cqa[idx].q0;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav1(void) {
    volatile uint32 s=0u; uint32 idx=s_cha1, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqa[idx].q1;idx=s_cqa[idx].q1;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav2(void) {
    volatile uint32 s=0u; uint32 idx=s_cha2, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqa[idx].q2;idx=s_cqa[idx].q2;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav3(void) {
    volatile uint32 s=0u; uint32 idx=s_cha3, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqa[idx].q3;idx=s_cqa[idx].q3;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav4(void) {
    volatile uint32 s=0u; uint32 idx=s_chb0, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqb[idx].q0;idx=s_cqb[idx].q0;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav5(void) {
    volatile uint32 s=0u; uint32 idx=s_chb1, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqb[idx].q1;idx=s_cqb[idx].q1;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav6(void) {
    volatile uint32 s=0u; uint32 idx=s_chb2, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqb[idx].q2;idx=s_cqb[idx].q2;}
    s_cq_result^=s; }
__attribute__((noinline))
void chase_trav7(void) {
    volatile uint32 s=0u; uint32 idx=s_chb3, n;
    for(n=0u;n<CQ_SIZE;n++){s+=s_cqb[idx].q3;idx=s_cqb[idx].q3;}
    s_cq_result^=s; }

/* Orchestrator — calls all eight traversals every iteration. */
__attribute__((noinline))
void chase_run_all(void)
{
    /* Eight independent volatile-chase traversals → ~7200 ns Loci timing */
    chase_trav0(); chase_trav1(); chase_trav2(); chase_trav3();
    chase_trav4(); chase_trav5(); chase_trav6(); chase_trav7();
}
