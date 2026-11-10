#ifndef GH_GS_INDEXED_LAYOUT_H
#define GH_GS_INDEXED_LAYOUT_H

#include <stdint.h>

/* Native coordinate mapping shared by the T32 and TIM3 upload readers. */
static inline uint64_t gh_gs_psmt8_offset(unsigned x, unsigned y, unsigned pitch) {
    unsigned block_x = (x % 128) / 16, block_y = (y % 64) / 16;
    unsigned local_x = x % 16, local_y = y % 16;
    unsigned block = (block_x & 1) ^ ((block_x & 2) << 1) ^ ((block_x & 4) << 2) ^
                     ((block_y & 1) << 1) ^ ((block_y & 2) << 2);
    unsigned column = ((local_x & 1) << 2) ^ ((local_x & 2) << 3) ^ ((local_x & 4) << 3) ^
                      ((local_x & 8) >> 2) ^ ((local_y & 1) << 3) ^
                      ((local_y & 2) ? 33 : 0) ^ ((local_y & 4) ? 96 : 0) ^ ((local_y & 8) << 4);
    return ((uint64_t)x / 128 + (uint64_t)(y / 64) * pitch) * 8192 + block * 256 + column;
}

static inline unsigned gh_gs_csm1_palette256(unsigned index) {
    return (index & ~24u) | ((index & 8) << 1) | ((index & 16) >> 1);
}

#endif
