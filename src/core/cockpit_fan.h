#ifndef GH_COCKPIT_FAN_H
#define GH_COCKPIT_FAN_H
#include "game_init.h"
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_cockpit_fan {
 gh_id_texture texture;float x[33],y[33],u[33],v[33];
 uint32_t color,bank,depth;uint8_t ready;
} gh_cockpit_fan;
/* Actual1430E8/15C410 33-vertex screen-space heat fan. SaveSystem's real
 * save pointer must be bound; finite normalized current-level progress only.
 * Explicit existing nativebinary32 rotation reference, no libc sin/cos. */
gh_id_error gh_cockpit_fan_prepare(const gh_id_state *,gh_core_save_view,
 gh_cockpit_fan *,uint32_t *frontier);
gh_id_error gh_cockpit_fan_texture_rgba(const gh_id_texture *,void *,size_t);
#ifdef __cplusplus
}
#endif
#endif
