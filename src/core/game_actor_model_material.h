#ifndef GH_GAME_ACTOR_MODEL_MATERIAL_H
#define GH_GAME_ACTOR_MODEL_MATERIAL_H
#include "game_actor_model_nodes.h"
#include "game_stage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {const gh_stage_material *material;uint64_t tex0;uint8_t source_member,selected_member,written,samples_display_base0;} gh_actor_stream_material;
typedef struct {gh_actor_model *model;gh_stage_textures *textures;size_t node_index,count;uint32_t parity;
 gh_actor_stream_material streams[16];uint8_t ready;} gh_actor_materials;
/* Actual14B4D0->148690 bounded TM3 profiles. Cache and image lifetime belong
 * to SAME stage.textures. Returns original material writes, not GPU upload.
 * Raw FF with an empty constructor slot retains the TEX0=0 display-page
 * sample. It has no TM3 image and no update write from 148690. */
gh_actor_model_status gh_actor_materials_0014b4d0(gh_actor_model *,unsigned,gh_stage_textures *,gh_actor_materials *,uint32_t *frontier);
/* 14B9A0 reuses the parity node's existing stream TEX0 and decoded owner.
 * Never allocates a missing cache entry or republishes material registers. */
gh_actor_model_status gh_actor_materials_existing_0014b9a0(gh_actor_model *,unsigned,
 gh_stage_textures *,gh_actor_materials *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
