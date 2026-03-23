#include "perf_asm.h"
#include "Bsp.h"

static sint16 asm_buf_a[128];
static sint16 asm_buf_b[128];
static sint16 asm_buf_dst[128];
static uint32 asm_mem_buf[256];

void asm_saturated_add_array(sint16 *dst, const sint16 *a, const sint16 *b, uint32 count)
{
    uint32 i;
    for (i = 0; i < count; i += 2) {
        uint32 va, vb, vr;
        __asm__ volatile ("ld.w %0, [%1]" : "=d"(va) : "a"(&a[i]));
        __asm__ volatile ("ld.w %0, [%1]" : "=d"(vb) : "a"(&b[i]));
        __asm__ volatile ("adds.h %0, %1, %2" : "=d"(vr) : "d"(va), "d"(vb));
        __asm__ volatile ("st.w [%0], %1" : : "a"(&dst[i]), "d"(vr));
    }
}

uint32 asm_crc32_byte(uint32 crc, uint8 byte)
{
    uint32 result;
    uint32 b = (uint32)byte;
    __asm__ volatile (
        "xor %0, %1, %2\n\t"
        : "=d"(result) : "d"(crc), "d"(b)
    );
    uint32 i;
    for (i = 0; i < 8; i++) {
        uint32 mask;
        __asm__ volatile (
            "and %0, %1, 1\n\t"
            "rsub %0, %0, 0\n\t"
            : "=d"(mask) : "d"(result)
        );
        __asm__ volatile (
            "sh %0, %1, -1\n\t"
            : "=d"(result) : "d"(result)
        );
        uint32 poly_masked;
        __asm__ volatile (
            "and %0, %1, %2\n\t"
            : "=d"(poly_masked) : "d"(mask), "d"(0xEDB88320u)
        );
        __asm__ volatile (
            "xor %0, %0, %1\n\t"
            : "+d"(result) : "d"(poly_masked)
        );
    }
    return result;
}

void asm_memory_fill(uint32 *dst, uint32 value, uint32 count)
{
    uint32 i;
    for (i = 0; i < count; i++) {
        __asm__ volatile ("st.w [%0], %1" : : "a"(&dst[i]), "d"(value));
    }
}

uint32 asm_count_leading_zeros(uint32 value)
{
    uint32 result;
    __asm__ volatile ("cls %0, %1" : "=d"(result) : "d"(value));
    return result;
}

uint32 asm_bit_reverse(uint32 value)
{
    uint32 result = 0;
    uint32 i;
    for (i = 0; i < 32; i++) {
        uint32 bit;
        __asm__ volatile ("extr.u %0, %1, %2, 1" : "=d"(bit) : "d"(value), "d"(i));
        uint32 pos = 31 - i;
        __asm__ volatile ("insert %0, %0, %1, %2, 1" : "+d"(result) : "d"(bit), "d"(pos));
    }
    return result;
}

void asm_packed_abs(sint16 *dst, const sint16 *src, uint32 count)
{
    uint32 i;
    for (i = 0; i < count; i += 2) {
        uint32 packed_in, packed_out;
        __asm__ volatile ("ld.w %0, [%1]" : "=d"(packed_in) : "a"(&src[i]));
        __asm__ volatile ("abs.h %0, %1" : "=d"(packed_out) : "d"(packed_in));
        __asm__ volatile ("st.w [%0], %1" : : "a"(&dst[i]), "d"(packed_out));
    }
}

void asm_run_all(void)
{
    uint32 i;
    volatile uint32 result;

    for (i = 0; i < 128; i++) {
        asm_buf_a[i] = (sint16)(i * 127 - 8000);
        asm_buf_b[i] = (sint16)(i * -63 + 4000);
    }

    asm_saturated_add_array(asm_buf_dst, asm_buf_a, asm_buf_b, 128);

    result = 0xFFFFFFFFu;
    for (i = 0; i < 64; i++) {
        result = asm_crc32_byte(result, (uint8)(i * 7));
    }

    asm_memory_fill(asm_mem_buf, 0xA5A5A5A5u, 256);
    result = asm_count_leading_zeros(0x00FF0000u);
    result = asm_bit_reverse(0xDEADBEEFu);
    asm_packed_abs(asm_buf_dst, asm_buf_a, 128);
}
