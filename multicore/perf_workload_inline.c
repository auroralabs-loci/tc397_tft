#include "Ifx_Types.h"
#include "Bsp.h"

#define MATRIX_SIZE 8
#define CRC_POLY 0xEDB88320u
#define WORKLOAD_ARRAY_SIZE 256

static uint32 matrix_a[MATRIX_SIZE][MATRIX_SIZE];
static uint32 matrix_b[MATRIX_SIZE][MATRIX_SIZE];
static uint32 matrix_c[MATRIX_SIZE][MATRIX_SIZE];
static uint32 sort_buffer[WORKLOAD_ARRAY_SIZE];

static inline __attribute__((always_inline)) void perf_matrix_multiply(volatile uint32 *result)
{
    uint32 i, j, k;
    uint32 sum = 0;
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix_a[i][j] = i + j + 1;
            matrix_b[i][j] = (i * MATRIX_SIZE) + j;
        }
    }
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            matrix_c[i][j] = 0;
            for (k = 0; k < MATRIX_SIZE; k++) {
                matrix_c[i][j] += matrix_a[i][k] * matrix_b[k][j];
            }
            sum += matrix_c[i][j];
        }
    }
    *result = sum;
}

static inline __attribute__((always_inline)) void perf_crc32_compute(const uint8 *data, uint32 len, volatile uint32 *result)
{
    uint32 crc = 0xFFFFFFFFu;
    uint32 i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC_POLY;
            else
                crc = crc >> 1;
        }
    }
    *result = ~crc;
}

static inline __attribute__((always_inline)) void perf_bubble_sort(uint32 *arr, uint32 len)
{
    uint32 i, j, tmp;
    for (i = 0; i < len - 1; i++) {
        for (j = 0; j < len - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

static inline __attribute__((always_inline)) void perf_memory_stress(volatile uint32 *dst, const uint32 *src, uint32 count)
{
    uint32 i;
    for (i = 0; i < count; i++) {
        dst[i] = src[i] ^ 0xA5A5A5A5u;
    }
    for (i = 0; i < count; i++) {
        dst[i] = dst[i] + src[count - 1 - i];
    }
}

static inline __attribute__((always_inline)) void perf_bitfield_stress(volatile uint32 *result, uint32 iterations)
{
    uint32 val = 0xDEADBEEFu;
    uint32 i;
    for (i = 0; i < iterations; i++) {
        val = ((val >> 7) & 0x01FFFFFFu) | ((val & 0x7Fu) << 25);
        val ^= (i * 0x9E3779B9u);
        val = (val << 13) | (val >> 19);
    }
    *result = val;
}

static inline __attribute__((always_inline)) uint32 perf_fibonacci_iterative(uint32 n)
{
    uint32 a = 0, b = 1, c, i;
    if (n == 0) return a;
    for (i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

static inline __attribute__((always_inline)) void perf_run_all_workloads(void)
{
    volatile uint32 result;
    uint8 test_data[64];
    uint32 i;

    for (i = 0; i < 64; i++) test_data[i] = (uint8)(i * 7);
    for (i = 0; i < WORKLOAD_ARRAY_SIZE; i++) sort_buffer[i] = WORKLOAD_ARRAY_SIZE - i;

    perf_matrix_multiply(&result);
    perf_crc32_compute(test_data, 64, &result);
    perf_bubble_sort(sort_buffer, WORKLOAD_ARRAY_SIZE);
    perf_memory_stress((volatile uint32 *)sort_buffer, (const uint32 *)matrix_a, MATRIX_SIZE * MATRIX_SIZE);
    perf_bitfield_stress(&result, 1000);
    result = perf_fibonacci_iterative(40);
}
