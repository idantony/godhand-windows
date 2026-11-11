#ifndef GH_GAME_ENEMY_ENTRY_H
#define GH_GAME_ENEMY_ENTRY_H
#include "game_enemy_initialize.h"
#include "game_enemy_manager.h"
#include "game_object_sounds.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {GH_ROOM_OBJECT_ENEMY=0x200};
typedef struct {gh_game_init_state *init;gh_room_pools *pools;gh_game_objects *room_objects,*global_objects;
 gh_boot_work *work;gh_heap *room_heap,*packet_heap;const void *fog_rgb;gh_core_save_view save;
 const uint8_t *scenario14,*borrowed586aa4;uint8_t *factory_serial;gh_enemy_se_banks *banks;
 gh_object_resource_bindings resources;gh_static_transform_policy policy;} gh_enemy_entry_bindings;
typedef struct {gh_enemy_entry_bindings b;gh_enemy_request request;int32_t manager_kind;uint32_t argument,stage,frontier,handle;
 int slot;uint8_t resource_from_room,room_reference,global_reference,published,preload_reference;
 gh_game_title_player object;gh_object_view resource;gh_object_resource_load load,global_load;gh_object_sound_load sound;
 gh_enemy_initialize initialize;} gh_enemy_entry;
/* Actual2905D0 resource/factory/manager/virtual9C ordered prefix for202/20A.
 * Original actor return remainsNULL until wholevirtualInit is recovered.
 * Current successful model prefix stops precisely28EBF0 with published=1. */
gh_actor_model_status gh_enemy_entry_002905d0(gh_enemy_entry *,const gh_enemy_entry_bindings *,const gh_enemy_request *,int32_t manager_kind,uint32_t argument,int *original_nonnull);
/* Entry-6 2A2 sibling. ready means slot/resource/actor publication completed;
 * the virtual 284AF8 provider still owns the original return. */
gh_actor_model_status gh_enemy_entry_prepare_2a2_002905d0(gh_enemy_entry *,
 const gh_enemy_entry_bindings *,const gh_enemy_request *,int32_t manager_kind,uint32_t argument,int *ready);
/* Nativeabort only ownpausedprefix, not original290770/virtual destructor. */
gh_actor_model_status gh_enemy_entry_abort_prefix(gh_enemy_entry *);
#ifdef __cplusplus
}
#endif
#endif
