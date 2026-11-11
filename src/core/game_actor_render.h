#ifndef GH_GAME_ACTOR_RENDER_H
#define GH_GAME_ACTOR_RENDER_H
#include "game_actor_model_draw.h"
#include "game_actor_shadow_draw.h"
#include "game_cockpit.h"
#include "game_effects_trans.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Native call glue, not another actor/model/OT owner. All inputs borrow the
 * existing room and Root globals. Unrecovered bodies remain required calls. */
typedef struct {
 const gh_boot_work *work;const gh_game_pl_camera *camera;
 const gh_game_world_light *light;const uint8_t *depth_shift;
 const void *elf;size_t elf_bytes;const gh_player_shadow *shadow;
 gh_game_scene_tables *actors;const gh_camera_follow *up_owner;
 const gh_player_shared_vector *pivot_owner;const gh_shadow_templates *templates;
 gh_stage_textures *textures;
 gh_shadow_emit emit_shadow;void *emit_context;
 gh_shadow_ray ray;void *ray_context;
 gh_actor_draw_callee draw_callee;void *draw_context;
 gh_actor_trans_callee other_callee;void *other_context;
 gh_static_transform_policy policy;
 gh_actor_draw_emit actor_emit;void *actor_emit_context;
 const gh_actor_light_globals *light_globals;
 const gh_title_player_view *marker_player;
 gh_core_save_view marker_save;
 gh_game_cockpit *marker_cockpit;
 gh_id_base **marker_id;
 gh_game_mt *marker_random;
 gh_id_runtime *marker_runtime;
 const gh_id_state *marker_manager;
 gh_effect_trans_emit marker_emit;
 void *marker_context;
} gh_actor_render_bindings;

typedef struct {
 gh_actor_render_bindings bindings;gh_actor_model *model;const void *identity;
 gh_actor_trans_call trans;gh_shadow_call shadow;gh_actor_draw_call draw;
 uint32_t marker_frontier;
 float marker_distance,marker_facing;
 uint8_t marker_cursor;
 uint8_t bound, marker_stage;
} gh_actor_render_call;

/* One original14A3E8, including recovered14EFA8 and14AC90 consumers. A failed
 * call retains all prefixes; next_call is allowed only after real completion
 * and permits selecting the current camera for the new invocation.
 * Missing packet consumers return their precise frontier, never draw success. */
gh_actor_model_status gh_actor_render_0014a3e8(gh_actor_render_call *,gh_actor_model *,
 const gh_actor_render_bindings *,uint32_t *frontier);
/* Original post-shadow marker prefix. Called under the 14A3E8 FEnv.
 * Borrows the existing stage material and Root ordering table owners. */
gh_actor_model_status gh_enemy_marker_after_shadow_0028edd0(gh_actor_render_call *,uint32_t *);
gh_actor_model_status gh_actor_render_next_call(gh_actor_render_call *);
#ifdef __cplusplus
}
#endif
#endif
