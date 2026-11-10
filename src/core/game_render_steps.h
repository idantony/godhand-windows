#ifndef GH_GAME_RENDER_STEPS_H
#define GH_GAME_RENDER_STEPS_H
#include "game_scene_ui.h"
#include "game_effects.h"
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_RENDER_STEP_OK,GH_RENDER_STEP_ARGUMENT,GH_RENDER_STEP_BOUNDS,
 GH_RENDER_STEP_FORMAT,GH_RENDER_STEP_UNSUPPORTED} gh_render_step_status;
typedef struct {
 float shift_x,shift_y,scale_x,scale_y,alpha,red,green,blue;
 uint32_t bank,depth;uint16_t clip,reserved;
} gh_render_draw_scalars;
typedef struct {
 gh_bundle_entry sprites,textures; /* original pointers0/4 become bounded views */
 gh_render_draw_scalars scalar;uint8_t ready;
} gh_render_draw_env;
/* Loads actual3C2FB0 ->5FEC20 through ELF32 PT_LOAD, including BSS. This is
 * the shared static object, not a second per-ID draw environment. Nonzero
 * serialized pointers are unsupported until their actual resource is bound. */
gh_render_step_status gh_render_draw_env_from_elf(gh_render_draw_env *,const void *,size_t);
/* Original2C54E8 preserves pointer fields0/4 and reserved32/33. */
gh_render_step_status gh_render_draw_init_002c54e8(gh_render_draw_env *);
/* These consumers return OK only for their verified original early/empty
 * paths. Nonempty shared messages and live eligible particles retain the
 * precise original consumer frontier; no arbitrary pointer traversal. */
gh_render_step_status gh_render_messages_002af028(const gh_game_scene_ui *,uint32_t *frontier);
gh_render_step_status gh_render_particles_002fb310(const gh_boot_work *,
 const gh_game_effects *,const uint8_t *canonical_esp,size_t esp_size,
 uint32_t *frontier,uint32_t *slot);
gh_render_step_status gh_render_cockpit_001410a8_gate(const gh_boot_work *,uint32_t *frontier);
gh_render_step_status gh_render_menu_00160b90_gate(const gh_boot_work *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
