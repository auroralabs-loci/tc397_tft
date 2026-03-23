#include "perf_heavy.h"
#include "Bsp.h"

static uint32 hm_a[HEAVY_MATRIX_SIZE][HEAVY_MATRIX_SIZE];
static uint32 hm_b[HEAVY_MATRIX_SIZE][HEAVY_MATRIX_SIZE];
static uint32 hm_c[HEAVY_MATRIX_SIZE][HEAVY_MATRIX_SIZE];
static uint32 hm_d[HEAVY_MATRIX_SIZE][HEAVY_MATRIX_SIZE];
static uint32 heavy_sort_buf[HEAVY_SORT_SIZE];
static uint32 heavy_mem_buf[1024];

void heavy_matrix_chain(volatile uint32 *result)
{
    uint32 i, j, k, sum = 0;
    for (i = 0; i < HEAVY_MATRIX_SIZE; i++)
        for (j = 0; j < HEAVY_MATRIX_SIZE; j++) {
            hm_a[i][j] = i * j + 1;
            hm_b[i][j] = (i + 1) * (j + 1);
        }
    for (i = 0; i < HEAVY_MATRIX_SIZE; i++)
        for (j = 0; j < HEAVY_MATRIX_SIZE; j++) {
            hm_c[i][j] = 0;
            for (k = 0; k < HEAVY_MATRIX_SIZE; k++)
                hm_c[i][j] += hm_a[i][k] * hm_b[k][j];
        }
    for (i = 0; i < HEAVY_MATRIX_SIZE; i++)
        for (j = 0; j < HEAVY_MATRIX_SIZE; j++) {
            hm_d[i][j] = 0;
            for (k = 0; k < HEAVY_MATRIX_SIZE; k++)
                hm_d[i][j] += hm_c[i][k] * hm_a[k][j];
            sum += hm_d[i][j];
        }
    *result = sum;
}

void heavy_sort_and_search(volatile uint32 *result)
{
    uint32 i, j, tmp, found = 0;
    for (i = 0; i < HEAVY_SORT_SIZE; i++)
        heavy_sort_buf[i] = (HEAVY_SORT_SIZE - i) * 17 + (i ^ 0xFF);
    for (i = 0; i < HEAVY_SORT_SIZE - 1; i++)
        for (j = 0; j < HEAVY_SORT_SIZE - i - 1; j++)
            if (heavy_sort_buf[j] > heavy_sort_buf[j + 1]) {
                tmp = heavy_sort_buf[j];
                heavy_sort_buf[j] = heavy_sort_buf[j + 1];
                heavy_sort_buf[j + 1] = tmp;
            }
    for (i = 0; i < HEAVY_SORT_SIZE; i++)
        if (heavy_sort_buf[i] == 12345) found = i;
    *result = found;
}

void heavy_hash_stress(volatile uint32 *result, uint32 iterations)
{
    uint32 h = 0x811C9DC5u;
    uint32 i;
    for (i = 0; i < iterations; i++) {
        h ^= i;
        h *= 0x01000193u;
        h ^= (h >> 16);
        h *= 0x85EBCA6Bu;
        h ^= (h >> 13);
    }
    *result = h;
}

void heavy_memory_thrash(volatile uint32 *dst, uint32 count)
{
    uint32 i;
    for (i = 0; i < count; i++)
        heavy_mem_buf[i] = i * 0x9E3779B9u;
    for (i = 0; i < count; i++)
        dst[i % count] = heavy_mem_buf[(i * 7) % count] ^ heavy_mem_buf[(i * 13) % count];
    for (i = count; i > 0; i--)
        dst[i - 1] += dst[i % count];
}

static uint32 fib_recursive(uint32 n)
{
    if (n <= 1) return n;
    return fib_recursive(n - 1) + fib_recursive(n - 2);
}

void heavy_recursive_fib(uint32 n, volatile uint32 *result)
{
    *result = fib_recursive(n);
}

void heavy_run_core0(void) {
    volatile uint32 r;
    heavy_matrix_chain(&r);
    heavy_hash_stress(&r, HEAVY_ITER_COUNT);
}
void heavy_run_core1(void) {
    volatile uint32 r;
    heavy_sort_and_search(&r);
    heavy_hash_stress(&r, HEAVY_ITER_COUNT);
}
void heavy_run_core2(void) {
    volatile uint32 r;
    heavy_memory_thrash((volatile uint32 *)heavy_mem_buf, 1024);
    heavy_recursive_fib(25, &r);
}
void heavy_run_core3(void) {
    volatile uint32 r;
    heavy_hash_stress(&r, HEAVY_ITER_COUNT * 2);
    heavy_matrix_chain(&r);
}
void heavy_run_core4(void) {
    volatile uint32 r;
    heavy_sort_and_search(&r);
    heavy_memory_thrash((volatile uint32 *)heavy_mem_buf, 512);
}
void heavy_run_core5(void) {
    volatile uint32 r;
    heavy_recursive_fib(22, &r);
    heavy_hash_stress(&r, HEAVY_ITER_COUNT);
}
