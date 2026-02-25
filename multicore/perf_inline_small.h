#ifndef PERF_INLINE_SMALL_H
#define PERF_INLINE_SMALL_H
#include "Ifx_Types.h"
static inline __attribute__((always_inline)) uint32 inl_small_hash(uint32 v) {
    v ^= v >> 16; v *= 0x45D9F3Bu; v ^= v >> 16; return v;
}
static inline __attribute__((always_inline)) uint32 inl_small_rotl(uint32 v, uint32 n) {
    return (v << n) | (v >> (32 - n));
}
static inline __attribute__((always_inline)) uint32 inl_small_mix(uint32 a, uint32 b) {
    return inl_small_hash(a) ^ inl_small_rotl(b, 13);
}
#endif
