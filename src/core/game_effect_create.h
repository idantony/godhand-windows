#ifndef GH_GAME_EFFECT_CREATE_H
#define GH_GAME_EFFECT_CREATE_H
#include "game_effect_release.h"
#include "game_actor_model.h"
#include "game_init.h"
#include "game_scene_tables.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual306528 integer ABI: bank/index/actor/config(NULL subset)/mode/selector.
 * selector==-1 denotes SAME actor, never a host pointer cast. */
typedef struct {uint32_t bank,index;const gh_title_player_view *actor;int32_t mode,selector;} gh_effect_create_request;
/* REL1640 matches work row +4 kind and +8 code. Return stable native views
 * for a live row, or both NULL for a completed original lookup with no row. */
typedef gh_effect_status (*gh_effect_event_actor_resolve)(void *,uint16_t kind,uint8_t actor_code,
 const gh_title_player_view **,const gh_actor_model **,uint32_t *frontier);
typedef struct {gh_effect_release_bindings storage;const gh_boot_work *work;
 gh_game_mt *random;const gh_actor_model *model;
 gh_effect_event_actor_resolve event_actor;void *event_actor_context;
 const float *atan_coeff;} gh_effect_create_bindings;
typedef struct {const void *identity;gh_game_effects *owner;gh_effect_create_request request;
 gh_effect_view est;uint32_t stage,row,count,slot,serial,frontier,config_flags;gh_game_mt *random_identity;int original_return;
 const gh_title_player_view *event_row_actor;const gh_actor_model *event_row_model;
 uint8_t active,complete,configuration_305e88,configuration_2fa158,configuration_nested_group;
 uint32_t configuration_nested_scale,configuration_nested_flags;
 const float *configuration_positioned_atan_coeff;
 uint32_t configuration_positioned_position[3],configuration_positioned_yaw;
 uint8_t configuration_positioned;} gh_effect_create_call;
/* New original invocation only; same call must retain its cursor on error. */
gh_effect_status gh_effect_create_begin(gh_effect_create_call *,const gh_effect_create_request *);
/* Only the known non-NULL305E88 configuration (unit scales/VF0 vectors,
 * flags40, actorNULL, mode=index+2200). Caller still owns the414 mark. */
/* Original explicit config from 2F1D3C; this bounded entry supports group >0
 * and does not run Event row actor lookup unless flags actually request it. */
gh_effect_status gh_effect_create_nested_begin_002f1d3c(gh_effect_create_call *,
 const gh_effect_create_request *,uint8_t group,uint32_t scale,uint32_t flags);
gh_effect_status gh_effect_create_ambient_begin_00305e88(gh_effect_create_call *,uint32_t index);
/* Original OM324 break, 1BFD28 -> 306528 with bank0/index1C, no actor.
 * Caller supplies the 84 virtual world position and 1F8180 yaw as binary32. */
gh_effect_status gh_effect_create_om324_break_begin_001bfd28(gh_effect_create_call *,
 const uint32_t world_xyz[3],uint32_t yaw_bits);
/* Original ordinary enemy impact 271810 -> 306838, bank58/index10,
 * actor NULL, unit scale, zero Euler, selector and mode -1. */
gh_effect_status gh_effect_create_enemy_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
/* Original 109928 part4 impact, bank0/index3, unit scale and zero Euler. */
gh_effect_status gh_effect_create_player_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
/* 2FA158's explicit configuration and camera cut -> bank 6 index. The
 * 2FA240 row actor is distinct from the original NULL selector argument. */
gh_effect_status gh_effect_create_event_cut_begin_002fa158(gh_effect_create_call *,uint32_t cut);
gh_effect_status gh_effect_event_cut_present_002fa128(const gh_game_effects *,uint32_t cut,int *present);
gh_effect_status gh_game_effect_create_00306528(gh_effect_create_call *,const gh_effect_create_bindings *);
typedef struct {gh_effect_view record;const gh_title_player_view *actor,*selector;
 const gh_actor_model *model;const gh_actor_part *part;uint32_t serial;
 uint8_t initialized,draw_padding215_known;
 /* SAME creator-owned slot, for the original per-frame consumer. Never a
  * duplicate work buffer; consumers also validate payload against storage. */
 uint8_t *payload;size_t payload_size;int32_t first_esp_child;} gh_effect_emitter_view;
gh_effect_status gh_effect_create_emitter_at(const gh_game_effects *,unsigned,gh_effect_emitter_view *);
/* Read same producer metadata for original3039E8 matching. Unknown external
 * slots stay strict, not assumed to have NULL actor. */
gh_effect_status gh_effect_create_actor_match(void *effects,int emitter,unsigned slot,
 const gh_title_player_view *,int *match,uint32_t *frontier);
/* The admitted EV01 bank-6 sprites borrow an actor root.
 * The emitter retains the selected part through a separate typed binding. */
int gh_effect_event_part_is_bound(gh_effect_view,const gh_title_player_view *,
 const gh_title_player_view *);
/* Actual1024-slot ESP companion. Verified type6/type1A constructors are admitted;
 * unknown actor domains remain strict before allocation. Serial binds the SAME
 * payload and source; initialized stays false until real virtualInit returns. */
struct gh_effects_frame_globals;
typedef struct {gh_effect_view record,texture,secondary_texture;const gh_title_player_view *actor,*selector;
 uint8_t *payload;size_t payload_size;uint32_t serial,emitter_serial;unsigned emitter_slot;uint8_t initialized,type;struct gh_effects_frame_globals *globals;gh_game_scene_tables *scene_actors;int32_t parent_emitter,previous,next;uint32_t parent_serial;/* Snapshot of known300458-zeroed source218, not a removedtoken test. */uint8_t target120_known_null;} gh_effect_esp_view;
gh_effect_status gh_effect_create_esp_allocate_002faa88(const gh_effect_release_bindings *,unsigned emitter_slot,uint32_t emitter_serial,unsigned *slot,uint32_t *serial,int *allocated,uint32_t *frontier);
gh_effect_status gh_effect_create_esp_at(const gh_game_effects *,unsigned,gh_effect_esp_view *);
/* Native cancellation of only this unfinished original virtualInit. Never a
 * replacement for original ESP destructor; ready/live slots are rejected. */
gh_effect_status gh_effect_create_esp_abort_partial(gh_game_effects *,unsigned,uint32_t);
/* Same canonical global companion, bound once per native effect owner. */
gh_effect_status gh_effect_create_bind_frame_globals(gh_game_effects *,struct gh_effects_frame_globals *);
gh_effect_status gh_effect_create_bind_frame_actors(gh_game_effects *,gh_game_scene_tables *);
/* Called only after actualtype6 virtualInit computation has returned1. */
gh_effect_status gh_effect_create_esp_commit_base(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_type6(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_14(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_12(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_13(gh_game_effects *,unsigned,uint32_t);
/* Non-SSD resource banks use the original one-based EST table index. */
gh_effect_status gh_effect_est_at_00306030(const gh_game_effects *,uint32_t,uint32_t,gh_effect_view *);
gh_effect_status gh_effect_create_esp_commit_18(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_1a(gh_game_effects *,unsigned,uint32_t);
/* Explicit host destruction after this slot's tasks/jobs stop; onlyverified
 * type6 NULLactor domain, never originalruntimeESP dtor. */
gh_effect_status gh_effect_create_esp_abort_native(gh_game_effects *,unsigned,uint32_t);
/* Bounded source texture views replace original3060B0 pointer fields. */
/* Same originalcNode links using slot+serial handles. Rawguest pointer words
 * stay removedzero tokens, never used to infer absence. Existingparent attach
 * genuinely returns withoutreparenting; unlink is original2B5EC0 only. */
gh_effect_status gh_effect_create_esp_attach_emitter_002b5d58(gh_game_effects *,unsigned,uint32_t,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_unlink_002b5ec0(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_bind_textures(gh_game_effects *,unsigned,uint32_t,gh_effect_view,gh_effect_view);
/* Native host owner destruction ONLY: detach metadata before actor/model and
 * raw buffers are destroyed; does NOT pretend302918 or clear original masks. */
void gh_effect_create_forget_on_owner_destroy(gh_game_effects *);
/* Restricted original ambientESP lifecycle; samepool/serial verified. */
gh_effect_status gh_effect_create_esp_pause_002fbe50(gh_game_effects *,unsigned,uint32_t,uint32_t *frontier);
struct gh_esp14_instance;
gh_effect_status gh_effect_create_esp_check_hidden_14(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_finish_hidden_14(struct gh_esp14_instance *,uint32_t *frontier);
/* Queries only the current active metadata, including paused/hidden slots.
 * Use the same canonical actor view until it reports no live references. */
gh_effect_status gh_effect_create_actor_has_live_refs(const gh_game_effects *,
 const gh_title_player_view *,int *borrowed);
gh_effect_status gh_effect_create_esp_destroy_hidden_native(gh_game_effects *,unsigned,uint32_t,uint32_t *frontier);
/* Check a backing allocation before freeing it. Includes paused/hidden slots
 * until their original destructor clears the active mask. */
gh_effect_status gh_effect_create_source_has_live_refs(const gh_game_effects *,
 const void *source,size_t bytes,int *borrowed);
#ifdef __cplusplus
}
#endif
#endif
