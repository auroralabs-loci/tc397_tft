#ifndef PERF_EXPLODE_H_
#define PERF_EXPLODE_H_

#include "Ifx_Types.h"

/* 30 GLOBAL extern noinline functions (21 workers + init + 8 traversals + orchestrator) */
void explode_poly_eval(void);
void explode_interp_linear(void);
void explode_fixed_point_mul(void);
void explode_running_stats(void);
void explode_threshold_count(void);

void explode_fnv1a_32(void);
void explode_djb2_hash(void);
void explode_crc8_byte(void);
void explode_checksum16(void);

void explode_memset32(void);
void explode_memcopy32(void);
void explode_memeq32(void);
void explode_memreverse32(void);

void explode_popcount_array(void);
void explode_parity_array(void);
void explode_bitrev32(void);
void explode_clz_sum(void);

void explode_insertion_sort(void);
void explode_binary_search(void);
void explode_merge_pass(void);

void explode_chains_init(void);
void explode_trav0(void); void explode_trav1(void); void explode_trav2(void); void explode_trav3(void);
void explode_trav4(void); void explode_trav5(void); void explode_trav6(void); void explode_trav7(void);
void explode_run_all(void);

#endif /* PERF_EXPLODE_H_ */
