#ifndef PERF_CHASE_H_
#define PERF_CHASE_H_

#include "Ifx_Types.h"

void chase_init_list(void);
void chase_traverse_forward(void);
void chase_traverse_reverse(void);
void chase_random_write(void);
void chase_volatile_sum(void);
void chase_checksum_validate(void);
void chase_extra_init(void);
void chase_extra0(void); void chase_extra1(void); void chase_extra2(void);
void chase_extra3(void); void chase_extra4(void); void chase_extra5(void);
void chase_run_all(void);

#endif /* PERF_CHASE_H_ */
