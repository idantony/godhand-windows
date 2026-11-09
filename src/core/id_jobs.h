#ifndef GH_ID_JOBS_H
#define GH_ID_JOBS_H
#include "id_base.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_id_error (*gh_id_post_message)(void *,uint16_t message);
typedef struct {uint32_t entry_index,address;uint8_t kind;} gh_id_frontier;
typedef struct {uint32_t entry_index;gh_id_entry *entry;uint32_t consumer_address;} gh_id_draw_job;
/* Actual Move: zero-rotation calcEntries path, frame wrap, five original
 * animation gates plus recovered Hermite color animation, icon posting. Unsupported
 * active animation/rotation reports exact consumer address, never fake success. */
gh_id_error gh_id_jobs_move(gh_id_base *,gh_id_post_message,void *,gh_id_frontier *);
/* Actual Trans visibility/order selection only. Collects real typed entries;
 * callers still execute setup and each consumer. This is not render completion.
 * No count-only/overflow partial success: capacity failure is explicit. */
gh_id_error gh_id_jobs_collect_visible(gh_id_base *,gh_id_draw_job *,size_t capacity,
 size_t *count,gh_id_frontier *);
typedef struct {gh_id_view source;uint16_t width,height;uint64_t tex0;} gh_id_texture;
/* Actual2ACDC8 TBL1 lookup, one-based. Restricted original T32/PSMT8 views. */
gh_id_error gh_id_texture_open(const gh_id_state *,uint32_t slot,uint32_t one_based,gh_id_texture *);
gh_id_error gh_id_texture_rgba(const gh_id_texture *,void *rgba,size_t capacity);
typedef struct {gh_id_texture texture;float x0,y0,x1,y1,u0,v0,u1,v1;
 uint32_t color;uint8_t color_mode;int16_t font_mode;uint16_t clip;} gh_id_texture_quad;
/* Actual TransTex->2C5EA8 observed MC path: centered alignment0, rotation0,
 * f80=1 and default drawInit scales/shift(+1,+1), clip reset0 by Trans.
 * Coordinates remain original pre-GS UI units; renderer scaling/state follows.
 * Caller runs Move first and preserves layer ordering from visible jobs. */
gh_id_error gh_id_texture_quad_prepare(const gh_id_state *,const gh_id_base *,
 const gh_id_entry *,gh_id_texture_quad *);
#ifdef __cplusplus
}
#endif
#endif
