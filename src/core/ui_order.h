#ifndef GH_UI_ORDER_H
#define GH_UI_ORDER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Semantic packet groups, not GS/DMA emulation. A font message is one group;
 * its glyph/upload/color commands retain their internal order. */
typedef struct gh_ui_order_group { uint32_t bank,depth; uint64_t identity; } gh_ui_order_group;
typedef enum gh_ui_order_status { GH_UI_ORDER_OK=0,GH_UI_ORDER_ARGUMENT,
 GH_UI_ORDER_CAPACITY,GH_UI_ORDER_UNSUPPORTED } gh_ui_order_status;
uint32_t gh_ui_order_bucket_count(uint32_t logical_bank);
/* Original 31A338 linkage and 14FF00 head insertion. Input order is enqueue
 * order. Outputs original group indices; unlinked banks are counted as omitted.
 * All input is validated before output mutation. depth0 is outside the safe
 * recovered domain (original computes a one-past bucket address). */
gh_ui_order_status gh_ui_order_indices(const gh_ui_order_group*,size_t count,
 uint32_t work_flags584,uint32_t work_mode578,size_t *indices,size_t capacity,
 size_t *written,size_t *omitted);
/* 31A538 submits before outer-loop counter increment / 31A338 reset. */
uint32_t gh_ui_order_submit_parity(uint32_t frame_counter);
uint32_t gh_ui_order_build_parity(uint32_t frame_counter);
#ifdef __cplusplus
}
#endif
#endif
