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
#include "game_player_post_state.h"
#include "game_collision_solid_step.h"
#include "game_terrain_sphere.h"
#include "game_player_ground_pose.h"
#include "game_actor_motion_solve.h"
#include "game_actor_finish_model.h"
#include "game_player_opacity.h"
#include "game_player_frame_tail.h"
#include "game_player_walk_state.h"
#include "game_player_run_state.h"
#ifdef __cplusplus
extern "C" {
#endif
struct gh_player_idle_input;struct gh_player_idle_input_bindings;
typedef enum {GH_PLAYER_FRAME_OK,GH_PLAYER_FRAME_ARGUMENT,GH_PLAYER_FRAME_BUSY,GH_PLAYER_FRAME_NUMERIC,GH_PLAYER_FRAME_UNSUPPORTED,GH_PLAYER_FRAME_CALLEE} gh_player_frame_status;
typedef struct {const gh_title_player_view *actor;int32_t kind;uint32_t parameter_bits,argument3,mode;int32_t selector;} gh_player_frame_effect_request;
/* Required actual306528 producer. Native status is separate from original
 * void return; only complete real effect creation may returnOK. */
typedef gh_player_frame_status (*gh_player_frame_effect_create)(void *,const gh_player_frame_effect_request *,uint32_t *frontier);
typedef gh_player_frame_status (*gh_player_frame_motion_step)(void *,const gh_player_motion_request *,int *original_ended,uint32_t *frontier);
/* Both real12BFB8 Door probes must complete before its caller flag write. */
typedef gh_player_frame_status (*gh_player_frame_door_probe)(void *,const gh_title_player_view *,uint32_t *frontier);
struct gh_player_frame_bindings;
typedef gh_player_frame_status (*gh_player_square_frame_consumer)(void *,const struct gh_player_frame_bindings *,uint32_t *);
typedef gh_player_frame_status (*gh_player_damage_positioned_effect)(void *,const uint32_t position[3],uint32_t *);
typedef gh_player_frame_status (*gh_player_damage_callee)(void *,const struct gh_player_frame_bindings *,uint32_t *);
typedef struct gh_player_frame_bindings {gh_game_title_player *player;gh_boot_work *work;gh_game_init_state *init;gh_game_pl_camera *camera;
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
 struct gh_player_idle_input *idle_input;const struct gh_player_idle_input_bindings *idle_input_bindings;
 const gh_game_player_node_cache *node_cache;
 gh_player_frame_door_probe door_probe;void *door_context;
 gh_game_scene_tables *actors; /* SAME canonical AKG/dynamic owner */
 gh_player_ground_globals *ground_globals;const struct gh_motion_ik_globals *ik_globals;const float *atan_coeff;
 gh_player_opacity_globals *opacity_globals;const gh_title_player_view *aux_68c;uint8_t auxiliary_68c_binding_known;
 gh_player_tail_target_slot *target_644;uint8_t *cockpit_flags_1ca0;
 gh_player_square_frame_consumer square_attack;void *square_context;
 gh_motion_sound_consumer damage_sound;void *damage_sound_context;
 gh_player_damage_positioned_effect damage_effect;
 gh_player_damage_callee damage_abort,damage_camera_exit;
 void *damage_context;
 uint8_t (*damage_vibration_rows)[16];
} gh_player_frame_bindings;
/* Native continuation only; all actual mutable fields live in borrowed owners.
 * Each source-prefix stage commits once. Error retry requires same bindings;
 * Legacy149EB8 errors latch until a fresh runtime to avoid prefix replay.
 * no input snapshot copy or shadow controller. Early original gates returnOK
 * without mutation. Complete ordinary frame will reset cursor for next call. */
typedef struct {gh_player_frame_bindings b;uint32_t stage,frontier;uint8_t bound,effect_pending;float previous_left,previous_right;int16_t left_x,left_y,right_x,right_y;uint8_t heat_stage,heat_ready;gh_player_frame_effect_request pending_effect;uint8_t damage_stage,damage_region_hit,give_index,give_phase;int32_t pending_damage;uint16_t pending_damage_kind;gh_motion_sound_request damage_sound_request;uint32_t damage_position[3];uint16_t hurt_sound_program;uint8_t hurt_phase;uint32_t state_entry;uint8_t idle_phase;size_t idle_motion_word,idle_sequence_word;gh_player_motion_request pending_motion;int motion_ended;uint8_t motion_step_complete,idle_end_prefix_done,idle_end_needs_random,input_started,post_take_index;int input_result;gh_game_player_post_state post_state;float previous_world_position[3];gh_collision_solid_frame solid_frame;
 gh_player_terrain_call wall_call,floor_call;gh_terrain_sphere_call sphere_call;
 gh_scene_ray_report wall_report,floor_report;float terrain_position[3],terrain_normal[3],floor_height;
 gh_player_ground_pose_call ground_pose;gh_motion_solve pose_solve;uint8_t model_failure_latched;gh_player_frame_status model_failure_status;uint32_t sequence_effect_index;
 gh_player_opacity_call opacity_call;gh_game_player_frame_tail tail;gh_actor_finish_model_call finish_model;
 gh_player_walk_call walk;
 gh_player_run_call run;
 uint8_t enemy_focus_fallback,enemy_focus_previous;
} gh_game_player_frame;
gh_player_frame_status gh_game_player_frame_00108ab0(gh_game_player_frame *,const gh_player_frame_bindings *);
/* Actual126820 +five-entry button history leaves1268B0/1268F0. */
gh_player_frame_status gh_game_player_button_history_00126820(gh_title_player_view *,const gh_boot_work *);
#ifdef __cplusplus
}
#endif
#endif
