#ifndef PERF_BLOAT_H_
#define PERF_BLOAT_H_

#include "Ifx_Types.h"

void bloat_matrix32_multiply(void);
void bloat_bubble_sort_large(void);
void bloat_hash_flood(void);
void bloat_memory_thrash(void);
void bloat_stride_scan(void);
void bloat_triple_nested_sum(void);
void bloat_chase_init(void);
void bloat_chase_traverse(void);
void bloat_run_all(void);

#endif /* PERF_BLOAT_H_ */
