#ifndef PERF_BRANCH_H_
#define PERF_BRANCH_H_

#include "Ifx_Types.h"

void branch_data_dependent_sort(void);
void branch_threshold_cascade(void);
void branch_bit_scatter(void);
void branch_search_unsorted(void);
void branch_early_exit_sabotage(void);
void branch_nested_dispatch(void);
void branch_volatile_chase(void);
void branch_run_all(void);

#endif /* PERF_BRANCH_H_ */
