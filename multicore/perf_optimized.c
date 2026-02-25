#include "perf_optimized.h"
#include "Bsp.h"

#define OPT_MATRIX_SIZE 8
#define OPT_ARRAY_SIZE 256

static uint32 opt_ma[OPT_MATRIX_SIZE][OPT_MATRIX_SIZE];
static uint32 opt_mb[OPT_MATRIX_SIZE][OPT_MATRIX_SIZE];
static uint32 opt_mc[OPT_MATRIX_SIZE][OPT_MATRIX_SIZE];
static uint32 opt_sort_buf[OPT_ARRAY_SIZE];

static const uint32 crc_table[256] = {
    0x00000000u,0x77073096u,0xEE0E612Cu,0x990951BAu,0x076DC419u,0x706AF48Fu,
    0xE963A5E0u,0x9E6495A7u,0x0EDB8832u,0x79DCB8A4u,0xE0D5E91Bu,0x97D2D988u,
    0x09B64C2Bu,0x7EB17CBDu,0xE7B82D09u,0x90BF1D9Fu,0x1DB71064u,0x6AB020F2u,
    0xF3B97148u,0x84BE41DEu,0x1ADAD47Du,0x6DDDE4EBu,0xF4D4B551u,0x83D385C7u,
    0x136C9856u,0x646BA8C0u,0xFD62F97Au,0x8A65C9ECu,0x14015C4Fu,0x63066CD9u,
    0xFA0F3D63u,0x8D080DF5u,0x3B6E20C8u,0x4C69105Eu,0xD56041E4u,0xA2677172u,
    0x3C03E4D1u,0x4B04D447u,0xD20D85FDu,0xA50AB56Bu,0x35B5A8FAu,0x42B2986Cu,
    0xDBBBC9D6u,0xACBCF940u,0x32D86CE3u,0x45DF5C75u,0xDCD60DCFu,0xABD13D59u,
    0x26D930ACu,0x51DE003Au,0xC8D75180u,0xBFD06116u,0x21B4F6B5u,0x56B3C423u,
    0xCFBA9599u,0xB8BDA50Fu,0x2802B89Eu,0x5F058808u,0xC60CD9B2u,0xB10BE924u,
    0x2F6F7C87u,0x58684C11u,0xC1611DABu,0xB6662D3Du,0x76DC4190u,0x01DB7106u,
    0x98D220BCu,0xEFD5102Au,0x71B18589u,0x06B6B51Fu,0x9FBFE4A5u,0xE8B8D433u,
    0x7807C9A2u,0x0F00F934u,0x9609A88Eu,0xE10E9818u,0x7F6A0D6Bu,0x086D3D2Du,
    0x91646C97u,0xE6635C01u,0x6B6B51F4u,0x1C6C6162u,0x856530D8u,0xF262004Eu,
    0x6C0695EDu,0x1B01A57Bu,0x8208F4C1u,0xF50FC457u,0x65B0D9C6u,0x12B7E950u,
    0x8BBEB8EAu,0xFCB9887Cu,0x62DD1DDFu,0x15DA2D49u,0x8CD37CF3u,0xFBD44C65u,
    0x4DB26158u,0x3AB551CEu,0xA3BC0074u,0xD4BB30E2u,0x4ADFA541u,0x3DD895D7u,
    0xA4D1C46Du,0xD3D6F4FBu,0x4369E96Au,0x346ED9FCu,0xAD678846u,0xDA60B8D0u,
    0x44042D73u,0x33031DE5u,0xAA0A4C5Fu,0xDD0D7822u,0x90D8B8E8u,0xE7D3A83Eu,
    0x7EDAB984u,0x09DD8912u,0x97B367B1u,0xE0B45727u,0x79BD0671u,0x0EBA36E7u,
    0x9EB9EF76u,0xE9BE8FE0u,0x70B7BF5Au,0x07B0BFCCu,0x99D4EA6Fu,0xEED3DAF9u,
    0x77DA8943u,0x00DD19D5u,0x8D6D6A3Eu,0xFA6C5AA8u,0x6365DB12u,0x14620B84u,
    0x8A068C27u,0xFD0198B1u,0x6408C90Bu,0x1301F99Du,0x830EB40Cu,0xF4090C9Au,
    0x6D008820u,0x1A07F8B6u,0x84639015u,0xF3648083u,0x6A6DD139u,0x1D6AE1AFu,
    0xCAAFF381u,0xBDA8C317u,0x24A192ADu,0x53A6A23Bu,0xCDC21798u,0xBACF270Eu,
    0x23C676B4u,0x54C14622u,0xC45E59B3u,0xB3596925u,0x2A50389Fu,0x5D572809u,
    0xC3139DAAu,0xB414AD3Cu,0x2D1D9C86u,0x5A1AAC10u,0xD71A82F5u,0xA01DB263u,
    0x391CE3D9u,0x4E1BD34Fu,0xD07F66ECu,0xA778567Au,0x3E7107C0u,0x49762756u,
    0xD9E930C7u,0xAEEE0051u,0x37E751EBu,0x40E0617Du,0xDE84E6DEu,0xA983D648u,
    0x3088B7F2u,0x478F8764u,0xE0D5E91Bu,0x97D2D988u,0x09B64C2Bu,0x7EB17CBDu,
    0xE7B82D09u,0x90BF1D9Fu,0x1DB71064u,0x6AB020F2u,0xF3B97148u,0x84BE41DEu,
    0x1ADAD47Du,0x6DDDE4EBu,0xF4D4B551u,0x83D385C7u,0x136C9856u,0x646BA8C0u,
    0xFD62F97Au,0x8A65C9ECu,0x14015C4Fu,0x63066CD9u,0xFA0F3D63u,0x8D080DF5u,
    0x3B6E20C8u,0x4C69105Eu,0xD56041E4u,0xA2677172u,0x3C03E4D1u,0x4B04D447u,
    0xD20D85FDu,0xA50AB56Bu,0x35B5A8FAu,0x42B2986Cu,0xDBBBC9D6u,0xACBCF940u,
    0x32D86CE3u,0x45DF5C75u,0xDCD60DCFu,0xABD13D59u,0x26D930ACu,0x51DE003Au,
    0xC8D75180u,0xBFD06116u,0x21B4F6B5u,0x56B3C423u,0xCFBA9599u,0xB8BDA50Fu,
    0x2802B89Eu,0x5F058808u,0xC60CD9B2u,0xB10BE924u,0x2F6F7C87u,0x58684C11u,
    0xC1611DABu,0xB6662D3Du,0x76DC4190u,0x01DB7106u,0x98D220BCu,0xEFD5102Au,
    0x71B18589u,0x06B6B51Fu,0x9FBFE4A5u,0xE8B8D433u,0x7807C9A2u,0x0F00F934u,
    0x9609A88Eu,0xE10E9818u,0x7F6A0D6Bu,0x086D3D2Du,0x91646C97u,0xE6635C01u,
    0x6B6B51F4u,0x1C6C6162u,0x856530D8u,0xF262004Eu,0x6C0695EDu,0x1B01A57Bu,
    0x8208F4C1u,0xF50FC457u,0x65B0D9C6u,0x12B7E950u
};

void opt_matrix_multiply(volatile uint32 *result)
{
    uint32 i, j, k, sum = 0;
    for (i = 0; i < OPT_MATRIX_SIZE; i++)
        for (j = 0; j < OPT_MATRIX_SIZE; j++) {
            opt_ma[i][j] = i + j + 1;
            opt_mb[j][i] = (i * OPT_MATRIX_SIZE) + j;
        }
    for (i = 0; i < OPT_MATRIX_SIZE; i++)
        for (j = 0; j < OPT_MATRIX_SIZE; j++) {
            uint32 acc = 0;
            for (k = 0; k < OPT_MATRIX_SIZE; k++)
                acc += opt_ma[i][k] * opt_mb[j][k];
            opt_mc[i][j] = acc;
            sum += acc;
        }
    *result = sum;
}

void opt_crc32_table(const uint8 *data, uint32 len, volatile uint32 *result)
{
    uint32 crc = 0xFFFFFFFFu;
    uint32 i;
    for (i = 0; i < len; i++)
        crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    *result = ~crc;
}

void opt_insertion_sort(uint32 *arr, uint32 len)
{
    uint32 i, j, key;
    for (i = 1; i < len; i++) {
        key = arr[i]; j = i;
        while (j > 0 && arr[j - 1] > key) { arr[j] = arr[j - 1]; j--; }
        arr[j] = key;
    }
}

void opt_memory_copy_aligned(uint32 *dst, const uint32 *src, uint32 count)
{
    uint32 i;
    for (i = 0; i + 3 < count; i += 4) {
        dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
    }
    for (; i < count; i++) dst[i] = src[i];
}

void opt_bitfield_fast(volatile uint32 *result, uint32 iterations)
{
    uint32 val = 0xDEADBEEFu, i;
    for (i = 0; i < iterations; i++) {
        val = ((val >> 7) | (val << 25)) ^ (i * 0x9E3779B9u);
        val = (val << 13) | (val >> 19);
    }
    *result = val;
}

uint32 opt_fibonacci(uint32 n)
{
    uint32 a = 0, b = 1, c, i;
    if (n == 0) return a;
    for (i = 2; i <= n; i++) { c = a + b; a = b; b = c; }
    return b;
}

void opt_run_all(void)
{
    volatile uint32 r;
    uint8 td[64]; uint32 i;
    for (i = 0; i < 64; i++) td[i] = (uint8)(i * 7);
    for (i = 0; i < OPT_ARRAY_SIZE; i++) opt_sort_buf[i] = OPT_ARRAY_SIZE - i;
    opt_matrix_multiply(&r);
    opt_crc32_table(td, 64, &r);
    opt_insertion_sort(opt_sort_buf, OPT_ARRAY_SIZE);
    opt_memory_copy_aligned(opt_sort_buf, (const uint32*)opt_ma, OPT_MATRIX_SIZE*OPT_MATRIX_SIZE);
    opt_bitfield_fast(&r, 1000);
    r = opt_fibonacci(40);
}
