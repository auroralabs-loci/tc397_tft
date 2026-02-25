#ifndef PERF_WORKLOAD_H
#define PERF_WORKLOAD_H

#include "Ifx_Types.h"

#define MATRIX_SIZE 8
#define CRC_POLY 0xEDB88320u
#define WORKLOAD_ARRAY_SIZE 256

void perf_matrix_multiply(volatile uint32 *result);
void perf_crc32_compute(const uint8 *data, uint32 len, volatile uint32 *result);
void perf_bubble_sort(uint32 *arr, uint32 len);
void perf_memory_stress(volatile uint32 *dst, const uint32 *src, uint32 count);
void perf_bitfield_stress(volatile uint32 *result, uint32 iterations);
uint32 perf_fibonacci_iterative(uint32 n);
void perf_run_all_workloads(void);

#endif
