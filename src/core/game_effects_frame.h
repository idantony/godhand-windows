#ifndef GH_GAME_EFFECTS_FRAME_H
#define GH_GAME_EFFECTS_FRAME_H
#include "game_effect_create.h"
#include "game_esp_create.h"
#include "game_scene_aux.h"
#include "game_scene_at_geometry.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fields absent from the existing canonical gh_game_effects owner; loaded
 * once from actual ELF. factor741DC0 is NOT754C48 or Main's game_rate. */
typedef struct gh_effects_frame_globals {uint32_t factor_741dc0,region_mask_741db0,region_active_741db8,emitter_count_3c3bc0;uint8_t initialized;uint32_t gaussian_offsets_61b540[32],gaussian_weights_61b5c0[32];} gh_effects_frame_globals;
gh_effect_status gh_effects_frame_globals_from_elf(gh_effects_frame_globals *,const void *,size_t);
/* Actual292F38 writes three distinct canonical fields. */
gh_effect_status gh_effects_frame_set_speed_00292f38(gh_effects_frame_globals *,gh_game_init_state *,uint32_t *canonical_ratio_754c48,uint32_t value_bits);
typedef struct {gh_effect_release_bindings storage;const gh_boot_work *work;gh_game_init_state *init;
 const gh_title_player_view *current_player;gh_effects_frame_globals *globals;
 gh_game_scene_aux *profile;gh_scene_aux_clock clock;void *clock_context;
 const float *atan_coeff;gh_static_transform_policy policy;
} gh_effects_frame_bindings;
typedef struct {gh_effects_frame_bindings b;const void *identity;uint32_t stage,frontier,emitter_count,esp_count;
 unsigned emitter_index,esp_index,pool_index,region_index,ssd_index;uint8_t bound,region_stage,ambient_pending;
 size_t ssd_offset,ssd_count;float region_position[3];uint32_t ambient_index;
 /* One original emitter call retains its prefix; no mutation replay on error. */
 gh_effect_emitter_view emitter;uint8_t emitter_stage;uint32_t emitter_serial;
 gh_effect_create_call ambient_call;uint8_t ambient_started;
 int32_t emitter_spawn_count;gh_esp_create_call esp_call;uint8_t esp_started;
} gh_game_effects_frame;
/* Same original profiler from scene_aux; completes and resets continuation
 * only after emitter -> region/SSD -> pool-age -> ESP ->factor1/end clock.
 * Unsupported live consumers never become empty success. */
gh_effect_status gh_game_effects_frame_update_0030f228(gh_game_effects_frame *,const gh_effects_frame_bindings *);
#ifdef __cplusplus
}
#endif
#endif
