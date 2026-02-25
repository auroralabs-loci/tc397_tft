#include "perf_workload_small.h"
#include "Bsp.h"

__attribute__((noinline)) void perf_small_crc(const uint8 *data, uint32 len, volatile uint32 *result)
{
    uint32 crc = 0xFFFFFFFFu;
    uint32 i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320u;
            else crc = crc >> 1;
        }
    }
    *result = ~crc;
}

__attribute__((noinline)) uint32 perf_small_fibonacci(uint32 n)
{
    uint32 a = 0, b = 1, c, i;
    if (n == 0) return a;
    for (i = 2; i <= n; i++) { c = a + b; a = b; b = c; }
    return b;
}

__attribute__((noinline)) void perf_small_bitrotate(volatile uint32 *result, uint32 iterations)
{
    uint32 val = 0xCAFEBABEu;
    uint32 i;
    for (i = 0; i < iterations; i++) {
        val = (val << 3) | (val >> 29);
        val ^= (i * 0x9E3779B9u);
    }
    *result = val;
}
