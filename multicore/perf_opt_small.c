#include "perf_opt_small.h"
static const uint32 sc_tbl[16] = {
    0x00000000u,0x1DB71064u,0x3B6E20C8u,0x26D930ACu,
    0x76DC4190u,0x6B6B51F4u,0x4DB26158u,0x50A5510Cu,
    0xEDB88320u,0xF00F9344u,0xD6D6A3E8u,0xCB61B38Cu,
    0x9B64C2B0u,0x86D3D2D4u,0xA00AE278u,0xBDBDF21Cu
};
void opt_small_crc_table(const uint8 *data, uint32 len, volatile uint32 *result)
{
    uint32 crc = 0xFFFFFFFFu, i;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        crc = sc_tbl[crc & 0x0F] ^ (crc >> 4);
        crc = sc_tbl[crc & 0x0F] ^ (crc >> 4);
    }
    *result = ~crc;
}
