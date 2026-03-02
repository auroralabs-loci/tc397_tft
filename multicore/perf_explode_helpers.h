#ifndef PERF_EXPLODE_HELPERS_H_
#define PERF_EXPLODE_HELPERS_H_

/* PERF-010: Symbol Explosion — inline helpers
 * All 8 functions below are static inline always_inline.
 * They will be INLINED AWAY — zero ELF symbols emitted for these 8.
 * This tests the analysis tool's ability to detect functions present in
 * source but absent from the binary symbol table. */

#include "Ifx_Types.h"

static inline __attribute__((always_inline))
uint32 explode_sat_add_u32(uint32 a, uint32 b)
{
    uint32 result = a + b;
    return (result < a) ? 0xFFFFFFFFu : result;
}

static inline __attribute__((always_inline))
void explode_swap_u32(volatile uint32 *a, volatile uint32 *b)
{
    uint32 tmp = *a;
    *a = *b;
    *b = tmp;
}

static inline __attribute__((always_inline))
uint32 explode_min_u32(uint32 a, uint32 b)
{
    return (a < b) ? a : b;
}

static inline __attribute__((always_inline))
uint32 explode_max_u32(uint32 a, uint32 b)
{
    return (a > b) ? a : b;
}

static inline __attribute__((always_inline))
uint32 explode_rotate_left(uint32 v, uint32 n)
{
    n &= 31u;
    return (v << n) | (v >> (32u - n));
}

static inline __attribute__((always_inline))
uint32 explode_rotate_right(uint32 v, uint32 n)
{
    n &= 31u;
    return (v >> n) | (v << (32u - n));
}

static inline __attribute__((always_inline))
uint32 explode_abs_diff(uint32 a, uint32 b)
{
    return (a >= b) ? (a - b) : (b - a);
}

static inline __attribute__((always_inline))
sint32 explode_round_q15(sint32 v)
{
    return (v + (1 << 14)) >> 15;
}

#endif /* PERF_EXPLODE_HELPERS_H_ */
