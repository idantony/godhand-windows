#ifndef GH_GAME_PLAYER_FACTORY_H
#define GH_GAME_PLAYER_FACTORY_H
#include "game_title_player.h"
#include "game_objects.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {GH_ROOM_OBJECT_PLAYER=0x100};
typedef struct {gh_game_title_player *player;gh_room_pools *pools;
 gh_game_objects *global_objects,*room_objects;const gh_boot_work *work;
 gh_disc *disc;gh_core_save_view save;uint8_t *factory_serial;} gh_player_factory_bindings;
typedef struct {gh_player_factory_bindings b;uint32_t stage,association_stage,frontier,handle;
 gh_object_view resource;size_t file_bytes;uint32_t resource_heap_original;
 uint8_t room_reference,global_reference,resource_from_room,complete;} gh_game_player_factory;
/* ActualCreateObj100/FFFF only: capacitygate,312F00,30FE68 commonfields,
 * SAMEDataManager acquire and30FCC0 two-listregistration. Success isfactory
 * completion only. Original31C4E0 mustpublishplayer THEN virtualInit44;
 * that nextrealmethod is107F58 (delta0), not implemented/acknowledgedhere. */
gh_title_player_status gh_game_player_factory_create_0030fa98(gh_game_player_factory *,const gh_player_factory_bindings *,int *original_nonnull);
/* Constructor-prefix/failedInit nativeabort cleanup ONLY. Not a fabricated
 * fullyinitializedgame virtualdestructor. Unregisters beforefreeingwrapper. */
gh_title_player_status gh_game_player_factory_release_prefix(gh_game_player_factory *);
/* Boundedtypedscalarview fordispatchers suchasSearchOM; owner+handleidentity
 * checked, never reinterpretcasta genericPS2 objectpointer. */
gh_title_player_status gh_game_player_factory_scalar_view(gh_game_player_factory *,uint32_t handle,const gh_title_player_view **);
#ifdef __cplusplus
}
#endif
#endif
