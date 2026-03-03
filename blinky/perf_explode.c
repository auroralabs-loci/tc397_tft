/* PERF-010: Symbol Explosion — Analysis Tool Stress via Mixed Symbol Overload
 *
 * ELF symbol categories produced by this file:
 *   GLOBAL (21): all extern noinline functions below
 *   LOCAL  (5):  the 5 static noinline helpers (explode_horner_step, etc.)
 *   INLINED (8): the 8 always_inline helpers in perf_explode_helpers.h
 *                (zero symbols emitted — stress-tests analysis detection)
 *
 * Primary purpose: test that analysis tools correctly count and categorise
 * GLOBAL, LOCAL, and inlined-away symbols when all three are present.
 * Secondary purpose: aggregate of 20 workers causes >100% response time
 * degradation from cumulative per-iteration overhead. */

#include "perf_explode.h"
#include "perf_explode_helpers.h"

/* ---- Working buffers (static, volatile) ---- */
static volatile sint32  s_xpl_poly_buf[128];
static volatile sint32  s_xpl_interp_buf[128];
static volatile sint32  s_xpl_q15_buf[64];
static volatile sint32  s_xpl_stats_buf[128];
static volatile sint32  s_xpl_thresh_buf[256];
static volatile uint8   s_xpl_hash_buf[128];
static volatile uint32  s_xpl_mem_src[256];
static volatile uint32  s_xpl_mem_dst[256];
static volatile uint32  s_xpl_sort_buf[64];
static volatile uint32  s_xpl_merge_a[64];
static volatile uint32  s_xpl_merge_b[64];
static volatile uint32  s_xpl_result;

/* ======== LOCAL (static noinline) helpers — 5 LOCAL symbols ======== */

static __attribute__((noinline))
sint32 explode_horner_step(sint32 acc, sint32 coeff, sint32 x)
{
    return acc * x + coeff;
}

static __attribute__((noinline))
void explode_welford_update(volatile sint32 *mean, volatile sint32 *m2,
                            sint32 n, sint32 val)
{
    sint32 delta  = val - *mean;
    *mean        += delta / n;
    *m2          += delta * (val - *mean);
}

static __attribute__((noinline))
void explode_merge_combine(volatile uint32 *dst, const volatile uint32 *a,
                           const volatile uint32 *b, sint32 n)
{
    sint32 i;
    for (i = 0; i < n; i++) {
        dst[i] = explode_sat_add_u32(a[i], b[i]);  /* uses always_inline */
    }
}

static __attribute__((noinline))
sint32 explode_lut_lookup(const volatile sint32 *lut, sint32 idx, sint32 size)
{
    if (idx < 0)    { return lut[0]; }
    if (idx >= size){ return lut[size - 1]; }
    return lut[idx];
}

static __attribute__((noinline))
uint32 explode_carry_fold(uint32 sum)
{
    /* Single-pass carry fold for 16-bit checksum */
    while (sum >> 16u) {
        sum = (sum & 0xFFFFu) + (sum >> 16u);
    }
    return sum;
}

/* ======== GLOBAL (extern noinline) workers — 21 GLOBAL symbols ======== */

/* --- Arithmetic (5) --- */

/* Degree-7 polynomial via Horner's method over 128 inputs.
 * Uses LOCAL helper explode_horner_step. */
__attribute__((noinline))
void explode_poly_eval(void)
{
    static const sint32 coeff[8] = {1, -2, 3, -4, 5, -6, 7, -8};
    sint32 i, j;
    for (i = 0; i < 128; i++) {
        sint32 x   = (sint32)(i + 1);
        sint32 acc = coeff[7];
        for (j = 6; j >= 0; j--) {
            acc = explode_horner_step(acc, coeff[j], x);
        }
        s_xpl_poly_buf[i] = acc;
    }
    s_xpl_result ^= (uint32)s_xpl_poly_buf[0];
}

/* Piecewise linear interpolation over 128 sample points. */
__attribute__((noinline))
void explode_interp_linear(void)
{
    sint32 i, idx;
    sint32 frac;
    for (i = 0; i < 128; i++) {
        /* fill source table */
        s_xpl_interp_buf[i] = (sint32)(i * i);
    }
    /* Interpolate at fractional positions */
    for (i = 0; i < 64; i++) {
        idx  = i * 2;
        frac = i;  /* simple integer fraction */
        s_xpl_poly_buf[i] = explode_lut_lookup(s_xpl_interp_buf, idx, 128)
                           + frac;
    }
    s_xpl_result ^= (uint32)s_xpl_poly_buf[0];
}

/* Q15 fixed-point multiply over 64 elements (uses always_inline round). */
__attribute__((noinline))
void explode_fixed_point_mul(void)
{
    sint32 i;
    for (i = 0; i < 64; i++) {
        sint32 a = (sint32)(i + 1) * 1000;
        sint32 b = (sint32)(64 - i) * 1000;
        s_xpl_q15_buf[i] = explode_round_q15(a * b);
    }
    s_xpl_result ^= (uint32)s_xpl_q15_buf[0];
}

/* Welford online mean/variance over 128 elements. Uses LOCAL helper. */
__attribute__((noinline))
void explode_running_stats(void)
{
    volatile sint32 mean = 0, m2 = 0;
    sint32 i;
    for (i = 0; i < 128; i++) {
        s_xpl_stats_buf[i] = (sint32)((i * 7u + 3u) & 0xFFFFu);
        explode_welford_update(&mean, &m2, i + 1, s_xpl_stats_buf[i]);
    }
    s_xpl_result ^= (uint32)(mean ^ m2);
}

/* Count elements in 256-element array within a threshold band. */
__attribute__((noinline))
void explode_threshold_count(void)
{
    volatile uint32 count = 0u;
    sint32 i;
    volatile sint32 lo = 100, hi = 200;
    for (i = 0; i < 256; i++) {
        s_xpl_thresh_buf[i] = (sint32)((i * 13u + 7u) & 0x1FFu);
        if (s_xpl_thresh_buf[i] >= lo && s_xpl_thresh_buf[i] <= hi) {
            count++;
        }
    }
    s_xpl_result ^= count;
}

/* --- Hash/CRC (4) --- */

__attribute__((noinline))
void explode_fnv1a_32(void)
{
    volatile uint32 hash = 2166136261u;
    sint32 i;
    for (i = 0; i < 128; i++) {
        s_xpl_hash_buf[i] = (uint8)(i & 0xFFu);
        hash ^= (uint32)s_xpl_hash_buf[i];
        hash *= 16777619u;
    }
    s_xpl_result ^= hash;
}

__attribute__((noinline))
void explode_djb2_hash(void)
{
    volatile uint32 hash = 5381u;
    sint32 i;
    for (i = 0; i < 64; i++) {
        s_xpl_hash_buf[i] = (uint8)('a' + (i % 26));
        hash = ((hash << 5u) + hash) ^ (uint32)s_xpl_hash_buf[i];
    }
    s_xpl_result ^= hash;
}

__attribute__((noinline))
void explode_crc8_byte(void)
{
    volatile uint8 crc = 0u;
    sint32 i, bit;
    for (i = 0; i < 256; i++) {
        crc ^= (uint8)(i & 0xFFu);
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x80u) {
                crc = (uint8)((uint8)(crc << 1u) ^ 0x07u);
            } else {
                crc = (uint8)(crc << 1u);
            }
        }
    }
    s_xpl_result ^= (uint32)crc;
}

/* 16-bit additive checksum with carry fold. Uses LOCAL helper. */
__attribute__((noinline))
void explode_checksum16(void)
{
    uint32 sum = 0u;
    sint32 i;
    for (i = 0; i < 256; i += 2) {
        uint32 word = ((uint32)(i & 0xFFu) << 8u) | (uint32)((i + 1) & 0xFFu);
        sum += word;
    }
    s_xpl_result ^= explode_carry_fold(sum);
}

/* --- Memory (4) --- */

__attribute__((noinline))
void explode_memset32(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_xpl_mem_src[i] = (uint32)(i * 0x01010101u);
    }
}

__attribute__((noinline))
void explode_memcopy32(void)
{
    sint32 i;
    for (i = 0; i < 256; i++) {
        s_xpl_mem_dst[i] = s_xpl_mem_src[i];
    }
}

__attribute__((noinline))
void explode_memeq32(void)
{
    volatile sint32 first_diff = -1;
    sint32 i;
    for (i = 0; i < 256; i++) {
        if (s_xpl_mem_dst[i] != s_xpl_mem_src[i]) {
            first_diff = i;
            break;
        }
    }
    s_xpl_result ^= (uint32)(first_diff + 1);
}

__attribute__((noinline))
void explode_memreverse32(void)
{
    sint32 i;
    for (i = 0; i < 128; i++) {
        /* uses always_inline swap */
        explode_swap_u32(&s_xpl_mem_dst[i], &s_xpl_mem_dst[255 - i]);
    }
}

/* --- Bit manipulation (4) --- */

__attribute__((noinline))
void explode_popcount_array(void)
{
    volatile uint32 total = 0u;
    sint32 i;
    uint32 v;
    for (i = 0; i < 128; i++) {
        v = s_xpl_mem_src[i];
        /* Software popcount */
        v = v - ((v >> 1u) & 0x55555555u);
        v = (v & 0x33333333u) + ((v >> 2u) & 0x33333333u);
        v = (v + (v >> 4u)) & 0x0F0F0F0Fu;
        total += (v * 0x01010101u) >> 24u;
    }
    s_xpl_result ^= total;
}

__attribute__((noinline))
void explode_parity_array(void)
{
    volatile uint8 parity = 0u;
    sint32 i;
    uint8  byte;
    for (i = 0; i < 256; i++) {
        byte = (uint8)(s_xpl_mem_src[i & 127u] >> (i & 24u));
        byte ^= byte >> 4u;
        byte ^= byte >> 2u;
        byte ^= byte >> 1u;
        parity ^= byte & 1u;
    }
    s_xpl_result ^= (uint32)parity;
}

/* Bit-reverse 64 uint32 values (uses always_inline rotate). */
__attribute__((noinline))
void explode_bitrev32(void)
{
    sint32 i;
    uint32 v;
    for (i = 0; i < 64; i++) {
        v = s_xpl_mem_src[i];
        /* Bit-reverse by nibble swaps then rotate */
        v = ((v & 0xAAAAAAAAu) >> 1u) | ((v & 0x55555555u) << 1u);
        v = ((v & 0xCCCCCCCCu) >> 2u) | ((v & 0x33333333u) << 2u);
        v = ((v & 0xF0F0F0F0u) >> 4u) | ((v & 0x0F0F0F0Fu) << 4u);
        v = explode_rotate_left(v, 16u);  /* uses always_inline */
        s_xpl_mem_dst[i] = v;
    }
}

__attribute__((noinline))
void explode_clz_sum(void)
{
    volatile uint32 total = 0u;
    sint32 i;
    uint32 v, clz;
    for (i = 0; i < 128; i++) {
        v = s_xpl_mem_src[i] | 1u;  /* ensure non-zero */
        /* Software CLZ */
        clz = 0u;
        if ((v & 0xFFFF0000u) == 0u) { clz += 16u; v <<= 16u; }
        if ((v & 0xFF000000u) == 0u) { clz += 8u;  v <<= 8u;  }
        if ((v & 0xF0000000u) == 0u) { clz += 4u;  v <<= 4u;  }
        if ((v & 0xC0000000u) == 0u) { clz += 2u;  v <<= 2u;  }
        if ((v & 0x80000000u) == 0u) { clz += 1u; }
        total += clz;
    }
    s_xpl_result ^= total;
}

/* --- Sort/search (3) --- */

__attribute__((noinline))
void explode_insertion_sort(void)
{
    sint32 i, j;
    uint32 key;
    /* Fill with descending values for worst-case insertion sort */
    for (i = 0; i < 64; i++) {
        s_xpl_sort_buf[i] = (uint32)(64 - i);
    }
    for (i = 1; i < 64; i++) {
        key = s_xpl_sort_buf[i];
        j   = i - 1;
        while (j >= 0 && s_xpl_sort_buf[j] > key) {
            s_xpl_sort_buf[j + 1] = s_xpl_sort_buf[j];
            j--;
        }
        s_xpl_sort_buf[j + 1] = key;
    }
    s_xpl_result ^= s_xpl_sort_buf[0];
}

__attribute__((noinline))
void explode_binary_search(void)
{
    volatile sint32 found = 0;
    sint32 i, lo, hi, mid;
    uint32 target;
    /* s_xpl_sort_buf is now sorted from explode_insertion_sort */
    for (i = 0; i < 16; i++) {
        target = (uint32)(i * 4u + 1u);
        lo = 0; hi = 63;
        while (lo <= hi) {
            mid = (lo + hi) / 2;
            if (s_xpl_sort_buf[mid] == target)      { found++; break; }
            else if (s_xpl_sort_buf[mid] < target)   { lo = mid + 1; }
            else                                      { hi = mid - 1; }
        }
    }
    s_xpl_result ^= (uint32)found;
}

/* Single merge pass of a 64-element merge sort. Uses LOCAL merge_combine. */
__attribute__((noinline))
void explode_merge_pass(void)
{
    sint32 i;
    for (i = 0; i < 64; i++) {
        s_xpl_merge_a[i] = (uint32)(i * 2u);
        s_xpl_merge_b[i] = (uint32)(i * 2u + 1u);
    }
    explode_merge_combine(s_xpl_mem_dst, s_xpl_merge_a, s_xpl_merge_b, 64);
    s_xpl_result ^= s_xpl_mem_dst[0];
}

/* ======== GLOBAL #22-#30 — eight independent volatile pointer-chase chains ====== */
/* 8 × 512 nodes × 16 B = 64 KB volatile BSS. Each chain has its own volatile
 * array and volatile head so Loci credits each traverse independently:
 * 8 × ~900 ns ≈ 7200 ns (>100% degradation).
 * Adds 9 GLOBAL symbols (init + 8 traversals), raising total GLOBAL count to 30.
 * LOCAL helper xpl_init_one adds 1 LOCAL symbol. */

#define XPL_C_SIZE 512u

typedef struct {
    volatile uint32 next_idx;
    volatile uint32 value;
    volatile uint32 pad0;
    volatile uint32 pad1;
} XplCNode; /* 16 bytes */

static volatile XplCNode s_xcn0[XPL_C_SIZE], s_xcn1[XPL_C_SIZE];
static volatile XplCNode s_xcn2[XPL_C_SIZE], s_xcn3[XPL_C_SIZE];
static volatile XplCNode s_xcn4[XPL_C_SIZE], s_xcn5[XPL_C_SIZE];
static volatile XplCNode s_xcn6[XPL_C_SIZE], s_xcn7[XPL_C_SIZE];

static volatile uint32 s_xh0, s_xh1, s_xh2, s_xh3;
static volatile uint32 s_xh4, s_xh5, s_xh6, s_xh7;

/* LOCAL — one additional LOCAL symbol in ELF */
__attribute__((noinline))
static void xpl_init_one(volatile XplCNode *nodes, volatile uint32 *head,
                         uint32 seed)
{
    uint32 perm[XPL_C_SIZE]; /* 2 KB stack */
    uint32 i, j, tmp;
    for (i = 0u; i < XPL_C_SIZE; i++) { perm[i] = i; nodes[i].value = i + 1u; }
    for (i = XPL_C_SIZE - 1u; i > 0u; i--) {
        seed = seed * 1664525u + 1013904223u;
        j    = seed % (i + 1u);
        tmp  = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (i = 0u; i < XPL_C_SIZE - 1u; i++) { nodes[perm[i]].next_idx = perm[i + 1u]; }
    nodes[perm[XPL_C_SIZE - 1u]].next_idx = perm[0u];
    *head = perm[0u];
}

/* GLOBAL #22 — Initialise all 8 chains before while(1). */
__attribute__((noinline))
void explode_chains_init(void)
{
    xpl_init_one(s_xcn0, &s_xh0, 0xFEEDFACEu);
    xpl_init_one(s_xcn1, &s_xh1, 0xDEADC0DEu);
    xpl_init_one(s_xcn2, &s_xh2, 0xBAADF00Du);
    xpl_init_one(s_xcn3, &s_xh3, 0xCAFED00Du);
    xpl_init_one(s_xcn4, &s_xh4, 0xACCE5500u);
    xpl_init_one(s_xcn5, &s_xh5, 0xD15EA5EDu);
    xpl_init_one(s_xcn6, &s_xh6, 0xB16B00B5u);
    xpl_init_one(s_xcn7, &s_xh7, 0xDEFEC8EDu);
}

/* GLOBAL #23-#30 — eight independent traverse functions */
__attribute__((noinline))
void explode_trav0(void) {
    volatile uint32 s=0u; uint32 idx=s_xh0, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn0[idx].value;idx=s_xcn0[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav1(void) {
    volatile uint32 s=0u; uint32 idx=s_xh1, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn1[idx].value;idx=s_xcn1[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav2(void) {
    volatile uint32 s=0u; uint32 idx=s_xh2, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn2[idx].value;idx=s_xcn2[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav3(void) {
    volatile uint32 s=0u; uint32 idx=s_xh3, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn3[idx].value;idx=s_xcn3[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav4(void) {
    volatile uint32 s=0u; uint32 idx=s_xh4, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn4[idx].value;idx=s_xcn4[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav5(void) {
    volatile uint32 s=0u; uint32 idx=s_xh5, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn5[idx].value;idx=s_xcn5[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav6(void) {
    volatile uint32 s=0u; uint32 idx=s_xh6, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn6[idx].value;idx=s_xcn6[idx].next_idx;}
    s_xpl_result^=s; }
__attribute__((noinline))
void explode_trav7(void) {
    volatile uint32 s=0u; uint32 idx=s_xh7, n;
    for(n=0u;n<XPL_C_SIZE;n++){s+=s_xcn7[idx].value;idx=s_xcn7[idx].next_idx;}
    s_xpl_result^=s; }

/* ======== Orchestrator — GLOBAL #31 ======== */

/* Calls all 29 workers above every while(1) iteration.
 * 8 always_inline helpers → absent from ELF symbol table (stress-tests tool). */
__attribute__((noinline))
void explode_run_all(void)
{
    /* Arithmetic */
    explode_poly_eval();
    explode_interp_linear();
    explode_fixed_point_mul();
    explode_running_stats();
    explode_threshold_count();
    /* Hash/CRC */
    explode_fnv1a_32();
    explode_djb2_hash();
    explode_crc8_byte();
    explode_checksum16();
    /* Memory */
    explode_memset32();
    explode_memcopy32();
    explode_memeq32();
    explode_memreverse32();
    /* Bit manipulation */
    explode_popcount_array();
    explode_parity_array();
    explode_bitrev32();
    explode_clz_sum();
    /* Sort/search */
    explode_insertion_sort();
    explode_binary_search();
    explode_merge_pass();
    /* Eight independent volatile-chase traversals → ~7200 ns Loci timing */
    explode_trav0(); explode_trav1(); explode_trav2(); explode_trav3();
    explode_trav4(); explode_trav5(); explode_trav6(); explode_trav7();
}
