#include "perf_light.h"
uint32 light_hash(uint32 seed, uint32 iterations)
{
    uint32 h = seed;
    uint32 i;
    for (i = 0; i < iterations; i++) {
        h ^= i;
        h *= 0x01000193u;
    }
    return h;
}
