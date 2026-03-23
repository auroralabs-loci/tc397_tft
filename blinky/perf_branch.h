#ifndef PERF_BRANCH_H_
#define PERF_BRANCH_H_

#include "Ifx_Types.h"

void branch_data_dependent_sort(void);
void branch_threshold_cascade(void);
void branch_bit_scatter(void);
void branch_search_unsorted(void);
void branch_early_exit_sabotage(void);
void branch_nested_dispatch(void);
void branch_chains_init(void);
void branch_trav0(void); void branch_trav1(void); void branch_trav2(void); void branch_trav3(void);
void branch_trav4(void); void branch_trav5(void); void branch_trav6(void); void branch_trav7(void);
void branch_run_all(void);

#endif /* PERF_BRANCH_H_ */
