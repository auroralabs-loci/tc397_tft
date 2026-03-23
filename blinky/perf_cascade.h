#ifndef PERF_CASCADE_H_
#define PERF_CASCADE_H_

#include "Ifx_Types.h"

/* Per-core compute workers (2 per core) */
void cascade_compute_core0(void);
void cascade_memory_core0(void);
void cascade_compute_core1(void);
void cascade_memory_core1(void);
void cascade_compute_core2(void);
void cascade_memory_core2(void);
void cascade_compute_core3(void);
void cascade_memory_core3(void);
void cascade_compute_core4(void);
void cascade_memory_core4(void);
void cascade_compute_core5(void);
void cascade_memory_core5(void);

/* Shared barrier */
void cascade_sync_barrier(void);

/* Volatile chase for core0 (>100% degradation) */
void cascade_chase_init(void);
void cascade_trav0(void); void cascade_trav1(void); void cascade_trav2(void); void cascade_trav3(void);
void cascade_trav4(void); void cascade_trav5(void); void cascade_trav6(void); void cascade_trav7(void);

/* Per-core orchestrators */
void cascade_run_core0(void);
void cascade_run_core1(void);
void cascade_run_core2(void);
void cascade_run_core3(void);
void cascade_run_core4(void);
void cascade_run_core5(void);

#endif /* PERF_CASCADE_H_ */
