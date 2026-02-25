#include "perf_asm_small.h"

uint32 asm_small_clz(uint32 value)
{
    uint32 result;
    __asm__ volatile ("cls %0, %1" : "=d"(result) : "d"(value));
    return result;
}
