#ifndef GH_GAME_PLAYER_CHILD_H
#define GH_GAME_PLAYER_CHILD_H
#include "game_player_factory.h"
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {GH_ROOM_OBJECT_PLAYER_CHILD=0x101,GH_PLAYER_CHILD_BYTES=0x20e0};
typedef struct {gh_room_pools *pools;gh_game_objects *global_objects,*room_objects;
 gh_boot_work *work;uint8_t *factory_serial;gh_object_resource_bindings resources;} gh_player_child_bindings;
typedef struct {gh_game_title_player object;gh_player_child_bindings b;
 gh_object_resource_load load;gh_object_view resource;gh_actor_model model;
 gh_bundle_entry geometry,texture;uint32_t stage,init_stage,frontier,handle;
 size_t file_bytes;uint8_t room_reference,global_reference,resource_from_room,complete;
 int model_original_return;} gh_game_player_child;
/* Actual312F70 only: originalzero20E0,shared14E990,44 VF0 records.
 * Constructorready does not mean100378 virtualInit succeeded. */
gh_title_player_status gh_game_player_child_construct_00312f70(gh_game_title_player *,gh_room_pools *,const gh_boot_work *,int *original_nonnull);
/* Actual101/FFFF CreateObj commonpostfactory; same serial/registries/resources.
 * Missingresource uses real DataManager load/wait when bindings supplied. */
gh_title_player_status gh_game_player_child_create_0030fa98(gh_game_player_child *,const gh_player_child_bindings *,int *original_nonnull);
/* Actual100378 prefix uses resource entries0/1, SAME native modelparts.
 * Current shared149738 returns precise148420; later149EB8/100870 never faked. */
gh_title_player_status gh_game_player_child_initialize_00100378(gh_game_player_child *,int *original_return);
/* Native incomplete-construction abort only, not original virtualdestructor.
 * Releases model before registry/resource/arena ownership. */
gh_title_player_status gh_game_player_child_release_prefix(gh_game_player_child *);
#ifdef __cplusplus
}
#endif
#endif
