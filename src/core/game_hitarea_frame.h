#ifndef GH_GAME_HITAREA_FRAME_H
#define GH_GAME_HITAREA_FRAME_H
#include "game_hitarea_shapes.h"
#include "game_ray_query.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Persist SAME cursor on late failure; never copy active cursor or reset its
 * borrowed canonical pool. Success resets cursor for the next original frame. */
typedef struct {const void *cursor_identity;const gh_game_init_state *identity;
 uint32_t generation,phase,slot,node,visited,slot_serial,target;uint8_t active,slot_started,query_done;} gh_hitarea_frame;
/* Explicit rate is SAME original Work574 semantic owner, not another clock.
 * Binary32 reference preserves FEnv. Positive hit list remains strict1FC810. */
typedef struct {gh_game_scene_tables *tables;const uint8_t *boot_elf;size_t boot_elf_size;const gh_title_player_view *current_player;const uint32_t *player_pause_words;} gh_hitarea_frame_bindings;
/* Initial values of the original data words at 3BF190/194. The root owns
 * the resulting words for the game lifetime, including any later writers. */
gh_hitarea_status gh_hitarea_pause_words_from_elf(const void *,size_t,uint32_t out[2]);
gh_hitarea_status gh_hitarea_object_last_hit_actor(const gh_hitarea_object *,const gh_title_player_view **);
gh_hitarea_status gh_hitarea_frame_resume_bound_001fdfd8(gh_game_init_state *,const gh_boot_work *,float,
 gh_static_transform_policy,gh_hitarea_frame *,const gh_hitarea_frame_bindings *,uint32_t *frontier);
gh_hitarea_status gh_hitarea_frame_resume_001fdfd8(gh_game_init_state *,const gh_boot_work *,float,
 gh_static_transform_policy,gh_hitarea_frame *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
