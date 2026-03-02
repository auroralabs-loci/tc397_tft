#ifndef PERF_CHASE_H_
#define PERF_CHASE_H_

#include "Ifx_Types.h"

void chase_init_list(void);
void chase_traverse_forward(void);
void chase_traverse_reverse(void);
void chase_random_write(void);
void chase_volatile_sum(void);
void chase_checksum_validate(void);
void chase_run_all(void);

#endif /* PERF_CHASE_H_ */
