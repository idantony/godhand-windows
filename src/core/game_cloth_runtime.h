#ifndef GH_GAME_CLOTH_RUNTIME_H
#define GH_GAME_CLOTH_RUNTIME_H
#include "game_cloth_templates.h"
#include "game_player_springs_frame.h"
#include "game_player_child_attach.h"
#include "game_ray_query.h"
#include "frame_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original mutable.data3BC730..3BC7CB plus BSS4615F0 vector. One persistent
 * owner, not perchild. Includes untouched data bytes so future writers share. */
typedef struct {uint8_t data[0x9c],wind[16];uint8_t loaded,initialized;} gh_cloth_runtime_globals;
gh_cloth_status gh_cloth_runtime_from_elf(gh_cloth_runtime_globals *,const void *,size_t);
gh_cloth_status gh_cloth_runtime_initialize_00107de0(gh_cloth_runtime_globals *,int init,int priority);
typedef struct {gh_game_player_springs *springs;const gh_player_child_attachment *attachment;
 gh_cloth_templates *templates;gh_cloth_runtime_globals *globals;gh_game_mt *random;gh_game_scene_tables *actors;
 gh_static_transform_policy matrix_policy;gh_frame_numeric_policy random_policy;} gh_cloth_step_bindings;
/* Borrowed operands of the common original 104010 body. */
typedef struct {
 gh_actor_model *model;
 const gh_actor_model *parent;
 uint8_t *config, *records;
 const uint8_t (*indices)[44];
 const uint32_t *coefficients[3];
 const gh_cloth_sphere *spheres;
 size_t sphere_count;
 unsigned count, tables;
} gh_cloth_operands;
/* Per-invocation resume metadata, not another spring/model owner. A nonzero
 * stage resumes that SAME originalcall; never reuse it as a new frame. */
typedef struct {gh_cloth_step_bindings bindings;gh_cloth_operands operands;const gh_cloth_geometry *geometry;
 float floor_y,radius_scale;uint32_t stage,frontier,random_words[2];uint8_t complete;gh_cloth_status phase_failure;} gh_cloth_step_prefix;
/* Actual104010 prefix through original10430C, including real107C68,106218,
 * sharedMT31D000,phasewrap and index-record90 clears. Normal positivecount
 * stops strict10430C before unported force integration; only originalcount0
 * returnscomplete. Never reports a whole simulation frame complete. */
gh_cloth_status gh_cloth_step_prefix_00104010(gh_cloth_step_prefix *,const gh_cloth_step_bindings *);
gh_cloth_status gh_cloth_step_operands_00104010(gh_cloth_step_prefix *,const gh_cloth_step_bindings *,
 const gh_cloth_operands *);
#ifdef __cplusplus
}
#endif
#endif
