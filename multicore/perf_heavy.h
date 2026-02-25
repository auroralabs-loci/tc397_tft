#ifndef PERF_HEAVY_H
#define PERF_HEAVY_H
#include "Ifx_Types.h"
#define HEAVY_MATRIX_SIZE 16
#define HEAVY_SORT_SIZE 512
#define HEAVY_ITER_COUNT 5000
void heavy_matrix_chain(volatile uint32 *result);
void heavy_sort_and_search(volatile uint32 *result);
void heavy_hash_stress(volatile uint32 *result, uint32 iterations);
void heavy_memory_thrash(volatile uint32 *dst, uint32 count);
void heavy_recursive_fib(uint32 n, volatile uint32 *result);
void heavy_run_core0(void);
void heavy_run_core1(void);
void heavy_run_core2(void);
void heavy_run_core3(void);
void heavy_run_core4(void);
void heavy_run_core5(void);
#endif
