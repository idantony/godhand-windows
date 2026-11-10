#ifndef GH_GAME_PLAYER_FRAME_H
#define GH_GAME_PLAYER_FRAME_H
#include "game_title_player.h"
#include "game_camera.h"
#include "game_scene_setup.h"
#include "game_effect_release.h"
#include "game_scene_at_damage.h"
#include "game_hitarea_shapes.h"
#include "game_collision_solid_units.h"
#include "game_player_init_tail.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_FRAME_OK,GH_PLAYER_FRAME_ARGUMENT,GH_PLAYER_FRAME_BUSY,GH_PLAYER_FRAME_NUMERIC,GH_PLAYER_FRAME_UNSUPPORTED,GH_PLAYER_FRAME_CALLEE} gh_player_frame_status;
typedef struct {const gh_title_player_view *actor;int32_t kind;uint32_t parameter_bits,argument3,mode;int32_t selector;} gh_player_frame_effect_request;
/* Required actual306528 producer. Native status is separate from original
 * void return; only complete real effect creation may returnOK. */
typedef gh_player_frame_status (*gh_player_frame_effect_create)(void *,const gh_player_frame_effect_request *,uint32_t *frontier);
typedef gh_player_frame_status (*gh_player_frame_motion_step)(void *,const gh_player_motion_request *,int *original_ended,uint32_t *frontier);
typedef struct {gh_game_title_player *player;gh_boot_work *work;gh_game_init_state *init;gh_game_pl_camera *camera;
 gh_core_save_view save;gh_scene_scenario_view scenario;const gh_effect_release_bindings *effects;
 const gh_title_player_view *current_player;const void *elf;size_t elf_bytes;gh_camera_numeric_policy numeric_policy;gh_player_frame_effect_create effect_create;void *effect_context;
 gh_scene_at_manager *scene_at;const gh_hitarea_object *take_object;gh_hitarea_object *give_objects;gh_game_collision_solids *solids;
 uint8_t take_binding_known,give_binding_known,give_present_mask;
 /* SAME factory/model/rate; original idle Begin requires blendFrames10.
  * Provider must preserve oldSEQ/current pose; never substitute blend0. */
 gh_actor_model *model;const uint8_t *resource;size_t resource_bytes;const float *rate;
 gh_player_motion_call initialize_motion;gh_player_frame_motion_step advance_motion;void *motion_context;
 /* Explicit original NULL identities only after publishers were audited.
  * Current positive auxiliary callers remain strict; raw0 is not evidence. */
 const gh_title_player_view *aux_698,*aux_6a0;uint8_t auxiliary_bindings_known;
} gh_player_frame_bindings;
/* Native continuation only; all actual mutable fields live in borrowed owners.
 * Each source-prefix stage commits once. Error retry requires same bindings;
 * no input snapshot copy or shadow controller. Early original gates returnOK
 * without mutation. Complete ordinary frame will reset cursor for next call. */
typedef struct {gh_player_frame_bindings b;uint32_t stage,frontier;uint8_t bound,effect_pending;float previous_left,previous_right;int16_t left_x,left_y,right_x,right_y;uint8_t heat_stage,heat_ready;gh_player_frame_effect_request pending_effect;uint8_t damage_stage,damage_region_hit,give_index,give_phase;int32_t pending_damage;uint16_t pending_damage_kind;uint32_t state_entry;uint8_t idle_phase;size_t idle_motion_word,idle_sequence_word;gh_player_motion_request pending_motion;int motion_ended;uint8_t motion_step_complete,idle_end_prefix_done,idle_end_needs_random;} gh_game_player_frame;
gh_player_frame_status gh_game_player_frame_00108ab0(gh_game_player_frame *,const gh_player_frame_bindings *);
/* Actual126820 +five-entry button history leaves1268B0/1268F0. */
gh_player_frame_status gh_game_player_button_history_00126820(gh_title_player_view *,const gh_boot_work *);
#ifdef __cplusplus
}
#endif
#endif
