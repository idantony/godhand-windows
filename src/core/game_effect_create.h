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
 uint8_t active,complete;} gh_effect_create_call;
/* New original invocation only; same call must retain its cursor on error. */
gh_effect_status gh_effect_create_begin(gh_effect_create_call *,const gh_effect_create_request *);
gh_effect_status gh_game_effect_create_00306528(gh_effect_create_call *,const gh_effect_create_bindings *);
typedef struct {gh_effect_view record;const gh_title_player_view *actor,*selector;
 const gh_actor_model *model;const gh_actor_part *part;uint32_t serial;
 uint8_t initialized,draw_padding215_known;} gh_effect_emitter_view;
gh_effect_status gh_effect_create_emitter_at(const gh_game_effects *,unsigned,gh_effect_emitter_view *);
/* Read same producer metadata for original3039E8 matching. Unknown external
 * slots stay strict, not assumed to have NULL actor. */
gh_effect_status gh_effect_create_actor_match(void *effects,int emitter,unsigned slot,
 const gh_title_player_view *,int *match,uint32_t *frontier);
/* Native host owner destruction ONLY: detach metadata before actor/model and
 * raw buffers are destroyed; does NOT pretend302918 or clear original masks. */
void gh_effect_create_forget_on_owner_destroy(gh_game_effects *);
#ifdef __cplusplus
}
#endif
#endif
