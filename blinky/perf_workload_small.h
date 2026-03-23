#ifndef PERF_WORKLOAD_SMALL_H
#define PERF_WORKLOAD_SMALL_H
#include "Ifx_Types.h"
void perf_small_crc(const uint8 *data, uint32 len, volatile uint32 *result);
uint32 perf_small_fibonacci(uint32 n);
void perf_small_bitrotate(volatile uint32 *result, uint32 iterations);
#endif
