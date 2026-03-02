/* PERF-007: Pointer Chase Labyrinth — Cache-Miss Induced Degradation
 * Volatile linked list traversal over 512 nodes with pseudo-random ordering.
 * Each hop forces a load from a non-adjacent cache line.
 * Designed to achieve >100% response time degradation via memory latency. */

#include "perf_chase.h"

/* Node structure: 16 bytes each → 512 nodes = 8 KB pool.
 * Sized to exceed L1 data cache (typically 8-16 KB on TC397 cores) when
 * combined with other live data, ensuring cold-cache on each traversal. */
typedef struct {
    volatile uint32 value;
    volatile uint32 next_idx;   /* index into s_chase_nodes[] */
    volatile uint32 prev_idx;   /* index for reverse traversal */
    volatile uint32 checksum;   /* per-node integrity word */
} ChaseNode;

#define CHASE_SIZE 512u

static volatile ChaseNode s_chase_nodes[CHASE_SIZE];
static volatile uint32    s_chase_fwd_head;   /* start index for forward walk */
static volatile uint32    s_chase_rev_head;   /* start index for reverse walk */
static volatile uint32    s_chase_ref_sum;    /* reference checksum from init */

/* LCG parameters for pseudo-random permutation */
#define LCG_A  1664525u
#define LCG_C  1013904223u

/* Build a pseudo-random permutation of [0..CHASE_SIZE-1] using an LCG.
 * Called ONCE before while(1). Produces two independent traversal orders. */
__attribute__((noinline))
void chase_init_list(void)
{
    uint32 i, j, tmp_idx, seed;
    uint32 perm[CHASE_SIZE];

    /* Initialize permutation to identity */
    for (i = 0u; i < CHASE_SIZE; i++) {
        perm[i] = i;
        s_chase_nodes[i].value    = i + 1u;
        s_chase_nodes[i].checksum = (i + 1u) ^ 0xA5A5A5A5u;
    }

    /* Fisher-Yates shuffle using LCG */
    seed = 0xDEADBEEFu;
    for (i = CHASE_SIZE - 1u; i > 0u; i--) {
        seed = seed * LCG_A + LCG_C;
        j = seed % (i + 1u);
        tmp_idx  = perm[i];
        perm[i]  = perm[j];
        perm[j]  = tmp_idx;
    }

    /* Wire up forward links following the permutation order */
    for (i = 0u; i < CHASE_SIZE - 1u; i++) {
        s_chase_nodes[perm[i]].next_idx = perm[i + 1u];
    }
    s_chase_nodes[perm[CHASE_SIZE - 1u]].next_idx = perm[0u]; /* wrap */
    s_chase_fwd_head = perm[0u];

    /* Build a SEPARATE reverse permutation using a different seed */
    seed = 0xCAFEBABEu;
    for (i = 0u; i < CHASE_SIZE; i++) {
        perm[i] = i;
    }
    for (i = CHASE_SIZE - 1u; i > 0u; i--) {
        seed = seed * LCG_A + LCG_C;
        j = seed % (i + 1u);
        tmp_idx  = perm[i];
        perm[i]  = perm[j];
        perm[j]  = tmp_idx;
    }
    for (i = 0u; i < CHASE_SIZE - 1u; i++) {
        s_chase_nodes[perm[i]].prev_idx = perm[i + 1u];
    }
    s_chase_nodes[perm[CHASE_SIZE - 1u]].prev_idx = perm[0u];
    s_chase_rev_head = perm[0u];

    /* Compute reference sum for validate function */
    s_chase_ref_sum = 0u;
    for (i = 0u; i < CHASE_SIZE; i++) {
        s_chase_ref_sum += s_chase_nodes[i].value;
    }
}

/* Walk all 512 nodes via next_idx pointers, accumulate checksum.
 * Every hop is a potential cache miss due to non-sequential access pattern. */
__attribute__((noinline))
void chase_traverse_forward(void)
{
    volatile uint32 sum = 0u;
    uint32 idx = s_chase_fwd_head;
    uint32 step;
    for (step = 0u; step < CHASE_SIZE; step++) {
        sum += s_chase_nodes[idx].value;
        idx  = s_chase_nodes[idx].next_idx;
    }
    /* Write back to defeat dead-code elimination */
    s_chase_nodes[idx].checksum = sum;
}

/* Walk all 512 nodes via prev_idx pointers (independent shuffle order).
 * Separate function ensures distinct GLOBAL symbol. */
__attribute__((noinline))
void chase_traverse_reverse(void)
{
    volatile uint32 sum = 0u;
    uint32 idx = s_chase_rev_head;
    uint32 step;
    for (step = 0u; step < CHASE_SIZE; step++) {
        sum += s_chase_nodes[idx].value;
        idx  = s_chase_nodes[idx].prev_idx;
    }
    s_chase_nodes[idx].checksum ^= sum;
}

/* Write to every 7th node in forward traversal order.
 * Stride-7 (prime) pointer hops make prefetch ineffective. */
__attribute__((noinline))
void chase_random_write(void)
{
    uint32 idx = s_chase_fwd_head;
    uint32 step;
    for (step = 0u; step < CHASE_SIZE; step++) {
        if ((step % 7u) == 0u) {
            s_chase_nodes[idx].value += 1u;
        }
        idx = s_chase_nodes[idx].next_idx;
    }
}

/* Accumulate all node values via forward pointer chain — cannot be cached
 * or reordered because the address of each access depends on the previous. */
__attribute__((noinline))
void chase_volatile_sum(void)
{
    volatile uint32 total = 0u;
    uint32 idx = s_chase_fwd_head;
    uint32 step;
    for (step = 0u; step < CHASE_SIZE; step++) {
        total += s_chase_nodes[idx].value;
        idx    = s_chase_nodes[idx].next_idx;
    }
    /* Prevent elimination: write total to a reachable location */
    s_chase_nodes[s_chase_fwd_head].checksum = total;
}

/* Re-traverse and compare sum against reference. Writes result flag. */
__attribute__((noinline))
void chase_checksum_validate(void)
{
    volatile uint32 total = 0u;
    uint32 idx = s_chase_fwd_head;
    uint32 step;
    for (step = 0u; step < CHASE_SIZE; step++) {
        total += s_chase_nodes[idx].value;
        idx    = s_chase_nodes[idx].next_idx;
    }
    /* Store match/mismatch flag — non-zero means corruption */
    s_chase_nodes[0u].checksum = (total == s_chase_ref_sum) ? 0u : 0xDEADu;
}

/* Bulk traverse: 8192 forward hops through the same linked list,
 * cycling multiple times.  Adds ~8192 serialised load-use latency steps
 * beyond the 5 standard workers, pushing total degradation above 100%. */
__attribute__((noinline))
void chase_bulk_traverse(void)
{
    volatile uint32 sum = 0u;
    uint32 idx = s_chase_fwd_head;
    uint32 step;
    for (step = 0u; step < 8192u; step++) {
        sum += s_chase_nodes[idx].value;
        idx  = s_chase_nodes[idx].next_idx;
    }
    s_chase_nodes[s_chase_fwd_head].checksum ^= sum;
}

/* Orchestrator — called every while(1) iteration. */
__attribute__((noinline))
void chase_run_all(void)
{
    chase_traverse_forward();
    chase_traverse_reverse();
    chase_random_write();
    chase_volatile_sum();
    chase_checksum_validate();
    chase_bulk_traverse();     /* extra 8192 hops for >100% degradation */
}
