#ifndef PERF_ASM_H
#define PERF_ASM_H
#include "Ifx_Types.h"
void asm_saturated_add_array(sint16 *dst, const sint16 *a, const sint16 *b, uint32 count);
uint32 asm_crc32_byte(uint32 crc, uint8 byte);
void asm_memory_fill(uint32 *dst, uint32 value, uint32 count);
uint32 asm_count_leading_zeros(uint32 value);
uint32 asm_bit_reverse(uint32 value);
void asm_packed_abs(sint16 *dst, const sint16 *src, uint32 count);
void asm_run_all(void);
#endif
