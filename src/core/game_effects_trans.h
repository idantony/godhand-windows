#ifndef GH_GAME_EFFECTS_TRANS_H
#define GH_GAME_EFFECTS_TRANS_H
#include "game_effects_frame.h"
#include "display_state.h"
#include "frame_state.h"
#include "ui_order.h"
#include "game_effect_sprite.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_EFFECT_SURFACE_SCENE_COLOR,GH_EFFECT_SURFACE_3900,GH_EFFECT_SURFACE_3A00,GH_EFFECT_SURFACE_3B00,GH_EFFECT_SURFACE_MATERIAL,GH_EFFECT_SURFACE_3C00} gh_effect_surface;
typedef enum {GH_EFFECT_PASS_CLEAR,GH_EFFECT_PASS_DOWNSAMPLE,GH_EFFECT_PASS_GAUSSIAN,GH_EFFECT_PASS_COMPOSITE,GH_EFFECT_PASS_SPRITE,GH_EFFECT_PASS_SCENE_WARP,GH_EFFECT_PASS_SCENE_CAPTURE} gh_effect_pass_kind;
typedef struct {int32_t x12_4,y12_4;uint32_t z;uint16_t u12_4,v12_4;uint32_t rgba;float q;float s,t;} gh_effect_trans_vertex;
typedef struct {uint8_t vertex_count;gh_effect_trans_vertex vertices[6];} gh_effect_trans_quad;
/* Restricted semantic nativepasses, not a generic GIF/GS interpreter. Surface
 * names refer to frontend-owned resources in the ONE renderer. Coordinates
 * preserve originalGS12.4 and FST UV12.4; sampler state is INHERITED (noTEX1
 * command in these producers). Packet order is enqueueorder, ONEOT sorts it. */
typedef struct {gh_ui_order_group order;uint32_t original_callee,slot,serial;
 gh_effect_pass_kind kind;gh_effect_surface source,destination;
 uint32_t frame_mask,prim;uint64_t alpha,test,zbuf,tex0;
 uint8_t writes_tex0,inherited_sampler,quad_count;
 uint8_t restores_scene_target,texture_flush_after;uint32_t restore_frame_mask;uint64_t restore_zbuf;
 gh_effect_trans_quad quads[25];
 uint8_t writes_clamp,writes_texa,writes_tex1,inherits_scene_target;uint64_t clamp,texa,tex1;const gh_stage_material *material;} gh_effect_trans_packet;
typedef gh_effect_status (*gh_effect_trans_emit)(void *,const gh_effect_trans_packet *);
typedef struct {gh_effect_release_bindings storage;const gh_boot_work *work;gh_effects_frame_globals *globals;const gh_display_state *display;
 gh_static_transform_policy policy;gh_frame_numeric_policy cvt_policy;
 gh_effect_trans_emit emit;void *context;
 const gh_game_pl_camera *camera;const gh_game_init_state *init;gh_stage_textures *textures;
 const struct gh_esp14_provider *model_effect;} gh_effects_trans_bindings;
typedef struct {const void *identity;gh_effects_trans_bindings b;unsigned esp_index;uint32_t serial,call_stage,frontier,enqueued;uint8_t bound,pending,complete,entered;uint32_t gate_10000;
 gh_effect_trans_packet packet;
 gh_effect_sprite_projection sprite;float sprite_fade;const gh_stage_material *sprite_material;} gh_game_effects_trans;
/* One actual2FB310 pass. Failed emit must leave no queued group; onOK the
 * provider owns a copy until ONEOT consumes/discards it. Retry retains pending
 * packet and never resubmits a completedprefix. */
gh_effect_status gh_effects_trans_begin_002fb310(gh_game_effects_trans *);
gh_effect_status gh_game_effects_trans_002fb310(gh_game_effects_trans *,const gh_effects_trans_bindings *);
#ifdef __cplusplus
}
#endif
#endif
