#ifndef PERF_OPTIMIZED_H
#define PERF_OPTIMIZED_H
#include "Ifx_Types.h"
void opt_matrix_multiply(volatile uint32 *result);
void opt_crc32_table(const uint8 *data, uint32 len, volatile uint32 *result);
void opt_insertion_sort(uint32 *arr, uint32 len);
void opt_memory_copy_aligned(uint32 *dst, const uint32 *src, uint32 count);
void opt_bitfield_fast(volatile uint32 *result, uint32 iterations);
uint32 opt_fibonacci(uint32 n);
void opt_run_all(void);
#endif
