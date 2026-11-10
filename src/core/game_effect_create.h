#ifndef GH_GAME_EFFECT_CREATE_H
#define GH_GAME_EFFECT_CREATE_H
#include "game_effect_release.h"
#include "game_actor_model.h"
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual306528 integer ABI: bank/index/actor/config(NULL subset)/mode/selector.
 * selector==-1 denotes SAME actor, never a host pointer cast. */
typedef struct {uint32_t bank,index;const gh_title_player_view *actor;int32_t mode,selector;} gh_effect_create_request;
typedef struct {gh_effect_release_bindings storage;const gh_boot_work *work;
 gh_game_mt *random;const gh_actor_model *model;} gh_effect_create_bindings;
typedef struct {const void *identity;gh_game_effects *owner;gh_effect_create_request request;
 gh_effect_view est;uint32_t stage,row,count,slot,serial,frontier,config_flags;gh_game_mt *random_identity;int original_return;
 uint8_t active,complete,configuration_305e88;} gh_effect_create_call;
/* New original invocation only; same call must retain its cursor on error. */
gh_effect_status gh_effect_create_begin(gh_effect_create_call *,const gh_effect_create_request *);
/* Only the known non-NULL305E88 configuration (unit scales/VF0 vectors,
 * flags40, actorNULL, mode=index+2200). Caller still owns the414 mark. */
gh_effect_status gh_effect_create_ambient_begin_00305e88(gh_effect_create_call *,uint32_t index);
gh_effect_status gh_game_effect_create_00306528(gh_effect_create_call *,const gh_effect_create_bindings *);
typedef struct {gh_effect_view record;const gh_title_player_view *actor,*selector;
 const gh_actor_model *model;const gh_actor_part *part;uint32_t serial;
 uint8_t initialized,draw_padding215_known;
 /* SAME creator-owned slot, for the original per-frame consumer. Never a
  * duplicate work buffer; consumers also validate payload against storage. */
 uint8_t *payload;size_t payload_size;} gh_effect_emitter_view;
gh_effect_status gh_effect_create_emitter_at(const gh_game_effects *,unsigned,gh_effect_emitter_view *);
/* Read same producer metadata for original3039E8 matching. Unknown external
 * slots stay strict, not assumed to have NULL actor. */
gh_effect_status gh_effect_create_actor_match(void *effects,int emitter,unsigned slot,
 const gh_title_player_view *,int *match,uint32_t *frontier);
/* Actual1024-slot ESP companion. Only verified type6 constructor is admitted;
 * unknown actor domains remain strict before allocation. Serial binds the SAME
 * payload and source; initialized stays false until real virtualInit returns. */
typedef struct {gh_effect_view record,texture,secondary_texture;const gh_title_player_view *actor,*selector;
 uint8_t *payload;size_t payload_size;uint32_t serial,emitter_serial;unsigned emitter_slot;uint8_t initialized,type;} gh_effect_esp_view;
gh_effect_status gh_effect_create_esp_allocate_002faa88(const gh_effect_release_bindings *,unsigned emitter_slot,uint32_t emitter_serial,unsigned *slot,uint32_t *serial,int *allocated,uint32_t *frontier);
gh_effect_status gh_effect_create_esp_at(const gh_game_effects *,unsigned,gh_effect_esp_view *);
/* Native cancellation of only this unfinished original virtualInit. Never a
 * replacement for original ESP destructor; ready/live slots are rejected. */
gh_effect_status gh_effect_create_esp_abort_partial(gh_game_effects *,unsigned,uint32_t);
/* Bounded source texture views replace original3060B0 pointer fields. */
gh_effect_status gh_effect_create_esp_bind_textures(gh_game_effects *,unsigned,uint32_t,gh_effect_view,gh_effect_view);
/* Native host owner destruction ONLY: detach metadata before actor/model and
 * raw buffers are destroyed; does NOT pretend302918 or clear original masks. */
void gh_effect_create_forget_on_owner_destroy(gh_game_effects *);
#ifdef __cplusplus
}
#endif
#endif
