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
 const gh_title_player_view *attachment_selector;
 const gh_title_player_view *event_row_actor;const gh_actor_model *event_row_model;
 uint8_t active,complete,configuration_305e88,configuration_2fa158,configuration_nested_group;
 uint32_t configuration_nested_scale,configuration_nested_flags;
 const float *configuration_positioned_atan_coeff;
 uint32_t configuration_positioned_position[3],configuration_positioned_yaw;
 float configuration_esp10_rotation[3],configuration_esp10_color[4];
 uint8_t configuration_positioned,configuration_part4_stun,configuration_death_part_zero,configuration_05288;
 uint8_t configuration_held_impact_part_zero,configuration_esp10;} gh_effect_create_call;
/* New original invocation only; same call must retain its cursor on error. */
gh_effect_status gh_effect_create_ev07_nested_002effec(gh_effect_create_call *,uint32_t tag,const float position[3],
 const float rotation[3],const float color[4],float scale);
gh_effect_status gh_effect_create_begin(gh_effect_create_call *,const gh_effect_create_request *);
gh_effect_status gh_effect_create_r101_weapon_002a8608(gh_effect_create_call *,
 const gh_title_player_view *weapon,const gh_title_player_view *selector,uint32_t index,uint32_t scale);
/* Only the known non-NULL305E88 configuration (unit scales/VF0 vectors,
 * flags40, actorNULL, mode=index+2200). Caller still owns the414 mark. */
/* Original explicit config from 2F1D3C; this bounded entry supports group >0
 * and does not run Event row actor lookup unless flags actually request it. */
gh_effect_status gh_effect_create_nested_begin_002f1d3c(gh_effect_create_call *,
 const gh_effect_create_request *,uint8_t group,uint32_t scale,uint32_t flags);
gh_effect_status gh_effect_create_ambient_begin_00305e88(gh_effect_create_call *,uint32_t index);
int gh_effect_ambient_row_is_bound(gh_game_effects *,uint32_t index,gh_effect_view);
/* Original 14FB48 explicit configuration for the 58:2D part15 attack. */
gh_effect_status gh_effect_create_actor_sequence_0014fb48(gh_effect_create_call *,
 const gh_effect_create_request *,uint32_t scale);
/* Original 14FB48 -> 306900, attached 58:02 with explicit part0
 * and otherwise identity configuration. */
gh_effect_status gh_effect_create_player_stun_part4_00306900(gh_effect_create_call *,const gh_title_player_view *);
gh_effect_status gh_effect_create_gate_guard_stun_part4_00306900(gh_effect_create_call *,const gh_title_player_view *);
gh_effect_status gh_effect_create_em0a_stun_part4_00306900(gh_effect_create_call *,const gh_title_player_view *);
gh_effect_status gh_effect_create_enemy_death_part0_00306900(gh_effect_create_call *,
 const gh_title_player_view *,int32_t mode);
/* 2A8608's original 58:2F/30 mode0, selector self, explicit part0. */
gh_effect_status gh_effect_create_enemy_knockdown_002a8608(gh_effect_create_call *,
 const gh_title_player_view *,uint32_t index);
/* Original OM324 break, 1BFD28 -> 306528 with bank0/index1C, no actor.
 * Caller supplies the 84 virtual world position and 1F8180 yaw as binary32. */
gh_effect_status gh_effect_create_om307_break_begin_00306838(gh_effect_create_call *,uint32_t index);
gh_effect_status gh_effect_create_om324_break_begin_001bfd28(gh_effect_create_call *,
 const uint32_t world_xyz[3],uint32_t yaw_bits);
/* Original OM346 split plank puff, 1A1950 -> bank0/index7. */
gh_effect_status gh_effect_create_om346_break_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
/* Original ordinary enemy impact 271810 -> 306838, bank58/index10,
 * actor NULL, unit scale, zero Euler, selector and mode -1. */
/* Paired-counter collateral reaches the original positioned CORE0:C. */
gh_effect_status gh_effect_create_enemy_amplified_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_enemy_boosted_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_enemy_deflect_begin_00306838(gh_effect_create_call *,
    const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_enemy_counter_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_enemy_blowaway_begin_00306838(gh_effect_create_call *,
   const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_enemy_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
/* Original 109928 part4 impact, bank0/index3, unit scale and zero Euler. */
/* Original grounded item sparkle, bank 0 and index 1B. */
gh_effect_status gh_effect_create_om_sparkle_begin_00306838(gh_effect_create_call *,const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_om30a_break_begin_0018c580(gh_effect_create_call *,const uint32_t[3]);
gh_effect_status gh_effect_create_r104_em03_ground_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_devil_projectile_impact_00306838(gh_effect_create_call *,const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_em03_projectile_impact_00306838(gh_effect_create_call *,const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_om331_break_begin_0019ca58(gh_effect_create_call *,const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_om305_break_begin_001c2ce0(gh_effect_create_call *,const uint32_t world_xyz[3]);
gh_effect_status gh_effect_create_player_impact_begin_00306838(gh_effect_create_call *,
 const uint32_t world_xyz[3]);
/* 2FA158's explicit configuration and camera cut -> bank 6 index. The
 * 2FA240 row actor is distinct from the original NULL selector argument. */
gh_effect_status gh_effect_create_event_cut_begin_002fa158(gh_effect_create_call *,uint32_t cut);
/* EV02's original bank1:1 parentless sequence, mode0, config flags2. */
gh_effect_status gh_effect_create_event_sequence_00005288(gh_effect_create_call *);
gh_effect_status gh_effect_event_cut_present_002fa128(const gh_game_effects *,uint32_t cut,int *present);
gh_effect_status gh_game_effect_create_00306528(gh_effect_create_call *,const gh_effect_create_bindings *);
typedef struct {gh_effect_view record;const gh_title_player_view *actor,*selector;
 const gh_actor_model *model;const gh_actor_part *part;uint32_t serial;
 uint8_t initialized,draw_padding215_known;
 /* SAME creator-owned slot, for the original per-frame consumer. Never a
  * duplicate work buffer; consumers also validate payload against storage. */
 uint8_t *payload;size_t payload_size;int32_t first_esp_child;} gh_effect_emitter_view;
int gh_effect_devil_emitter_is_bound(const gh_game_effects *,const gh_effect_emitter_view *);
int gh_effect_em03_emitter_is_bound(const gh_game_effects *,const gh_effect_emitter_view *);
int gh_effect_emad_emitter_is_bound(const gh_game_effects *,const gh_effect_emitter_view *);
int gh_effect_player_stun_row(gh_effect_view);
int gh_effect_player_attachment_emitter_is_bound(const gh_effect_emitter_view *);
int gh_effect_player_heat_root_emitter_is_bound(const gh_effect_emitter_view *);
int gh_effect_r101_weapon_emitter_is_bound(const gh_effect_emitter_view *);
int gh_effect_stamp19_attack_part_is_bound(const gh_effect_emitter_view *);
int gh_effect_r103_slide_camera_emitter_is_bound(const gh_effect_emitter_view *);
int gh_effect_em00_attack_emitter_is_bound(const gh_effect_emitter_view *);
int gh_effect_knockdown_emitter_is_bound(const gh_effect_emitter_view *);
int gh_effect_enemy_light_emitter_is_bound(const gh_game_effects *,const gh_effect_emitter_view *);
int gh_effect_stamp19_death_part_is_bound(const gh_effect_emitter_view *);
gh_effect_status gh_effect_create_emitter_at(const gh_game_effects *,unsigned,gh_effect_emitter_view *);
/* Read same producer metadata for original3039E8 matching. Unknown external
 * slots stay strict, not assumed to have NULL actor. */
gh_effect_status gh_effect_create_actor_match(void *effects,int emitter,unsigned slot,
 const gh_title_player_view *,int *match,uint32_t *frontier);
/* The admitted EV01 bank-6 sprites borrow an actor root.
 * The emitter retains the selected part through a separate typed binding. */
int gh_effect_r101_door_break_part_is_bound(gh_effect_view,const gh_title_player_view *,
 const gh_title_player_view *);
int gh_effect_ev05_partless_is_bound(gh_effect_view,const gh_title_player_view *,const gh_title_player_view *);
int gh_effect_event_part_is_bound(gh_effect_view,const gh_title_player_view *,
 const gh_title_player_view *);
/* Actual1024-slot ESP companion. Verified type6/type1A constructors are admitted;
 * unknown actor domains remain strict before allocation. Serial binds the SAME
 * payload and source; initialized stays false until real virtualInit returns. */
struct gh_effects_frame_globals;
typedef struct {
 uint32_t flags;
 float tint_alpha;
 uint8_t opacity[3],valid;
} gh_effect_retired_actor;
typedef struct {gh_effect_view record,texture,secondary_texture;const gh_title_player_view *actor,*selector;const gh_actor_part *light_part;
 uint8_t *payload;size_t payload_size;uint32_t serial,emitter_serial;unsigned emitter_slot;uint8_t initialized,type;struct gh_effects_frame_globals *globals;gh_game_scene_tables *scene_actors;int32_t parent_emitter,previous,next;uint32_t parent_serial;/* Snapshot of known300458-zeroed source218, not a removedtoken test. */uint8_t target120_known_null;gh_effect_retired_actor retired_actor;} gh_effect_esp_view;
int gh_effect_attack_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_em00_attack_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_model_esp_actor_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
gh_effect_status gh_effect_create_esp_devil_trail_init_002f5788(gh_game_effects *,unsigned,uint32_t,
 uint32_t,gh_static_transform_policy,uint32_t *);
gh_effect_status gh_effect_create_esp_devil_trail_move_002f5bf8(gh_game_effects *,unsigned,uint32_t,
 const gh_boot_work *,gh_static_transform_policy,uint32_t *);
int gh_effect_devil_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_enemy_light_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_em03_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_emad_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_r100_om307_esp04_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_player_attachment_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_knockdown_esp_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
int gh_effect_esp_trail_is_bound(const gh_game_effects *,const gh_effect_esp_view *);
/* Borrow the canonical room arenas; release effects before tearing them down. */
gh_effect_status gh_effect_create_bind_trail_pools(gh_game_effects *,gh_room_pools *);
gh_effect_status gh_effect_create_esp_trail_init_002ebf98(gh_game_effects *,unsigned,uint32_t,
 uint32_t work594,const float position[4],uint32_t *frontier);
gh_effect_status gh_effect_create_esp_trail_sample_002ec218(gh_game_effects *,unsigned,uint32_t,
 const float position[4],uint32_t *frontier);
gh_effect_status gh_effect_create_esp_trail_view(const gh_game_effects *,unsigned,uint32_t,
 const float **positions,unsigned *count);
gh_effect_status gh_effect_create_esp_trail_edges_002f62f8(gh_game_effects *,unsigned,uint32_t,
 uint32_t work594,gh_static_transform_policy,uint32_t *frontier);
gh_effect_status gh_effect_create_esp_trail_edges_002f6740(gh_game_effects *,unsigned,uint32_t,
 gh_static_transform_policy,uint32_t *frontier);
gh_effect_status gh_effect_create_esp_trail_view_edges(const gh_game_effects *,unsigned,uint32_t,
 const float **positions,const float **other_edge,unsigned *count);
gh_effect_status gh_effect_create_esp_allocate_002faa88(const gh_effect_release_bindings *,unsigned emitter_slot,uint32_t emitter_serial,unsigned *slot,uint32_t *serial,int *allocated,uint32_t *frontier);
gh_effect_status gh_effect_create_esp_at(const gh_game_effects *,unsigned,gh_effect_esp_view *);
/* Native cancellation of only this unfinished original virtualInit. Never a
 * replacement for original ESP destructor; ready/live slots are rejected. */
gh_effect_status gh_effect_create_esp_abort_partial(gh_game_effects *,unsigned,uint32_t);
/* Same canonical global companion, bound once per native effect owner. */
gh_effect_status gh_effect_create_bind_frame_globals(gh_game_effects *,struct gh_effects_frame_globals *);
gh_effect_status gh_effect_create_bind_frame_actors(gh_game_effects *,gh_game_scene_tables *);
/* Called only after actualtype6 virtualInit computation has returned1. */
gh_effect_status gh_effect_create_esp_commit_type5(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_base(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_type6(gh_game_effects *,unsigned,uint32_t);
/* R003 SSD bank3/index0 parentless class08, original 2FFC20/2EA908. */
int gh_effect_r003_esp08_row(gh_effect_view);
int gh_effect_esp08_grid_row(gh_effect_view);
int gh_effect_esp08_screen_row(gh_effect_view);
gh_effect_status gh_effect_create_esp_commit_8(gh_game_effects *,unsigned,uint32_t);
int gh_effect_esp_type7_record_bound(const gh_game_effects *,const gh_effect_esp_view *);
gh_effect_status gh_effect_create_esp_commit_type7(gh_game_effects *,unsigned slot,uint32_t serial);
unsigned gh_effect_esp_type15_copies(const gh_game_effects *,const gh_effect_esp_view *);
gh_effect_status gh_effect_create_esp_commit_15(gh_game_effects *,unsigned slot,uint32_t serial);
gh_effect_status gh_effect_create_esp_commit_14(gh_game_effects *,unsigned,uint32_t);
gh_effect_status gh_effect_create_esp_commit_10(gh_game_effects *,unsigned,uint32_t);
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
gh_effect_status gh_effect_create_esp_heat_aura_init_002e35c0(gh_game_effects *,unsigned,uint32_t,
 uint32_t work594,uint32_t *frontier);
gh_effect_status gh_effect_create_esp_heat_aura_move_002e4280(gh_game_effects *,unsigned,uint32_t,
 gh_static_transform_policy,uint32_t *frontier);
struct gh_esp03_active_state;
const struct gh_esp03_active_state *gh_effect_create_esp_heat_aura_state(
 const gh_game_effects *,unsigned,uint32_t serial);
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
/* Native deletion state for the original unlinked CORE58:02 death sprite.
 * Other effect borrows still have to clear before the actor is freed. */
/* Release host attachments after their original effects stop reading the actor. */
gh_effect_status gh_effect_create_prepare_actor_retirement(gh_game_effects *,const gh_title_player_view *);
gh_effect_status gh_effect_create_detach_ev07_stopped_effects(gh_game_effects *,const gh_title_player_view *);
gh_effect_status gh_effect_create_actor_has_live_refs(const gh_game_effects *,
 const gh_title_player_view *,int *borrowed);
gh_effect_status gh_effect_create_esp_destroy_hidden_native(gh_game_effects *,unsigned,uint32_t,uint32_t *frontier);
/* Check a backing allocation before freeing it. Includes paused/hidden slots
 * until their original destructor clears the active mask. */
/* State5 no longer visits the effect frame. Preserve original packed slots
 * until its reset, while releasing validated inactive D7 host borrows. */
gh_effect_status gh_effect_create_quiesce_actor_room_exit(gh_game_effects *,const gh_title_player_view *);
gh_effect_status gh_effect_create_source_has_live_refs(const gh_game_effects *,
 const void *source,size_t bytes,int *borrowed);
#ifdef __cplusplus
}
#endif
#endif
