#ifndef GH_GAME_PLAYER_INIT_H
#define GH_GAME_PLAYER_INIT_H
#include "game_player_factory.h"
#include "../assets/model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_INIT_OK,GH_PLAYER_INIT_ARGUMENT,GH_PLAYER_INIT_BOUNDS,GH_PLAYER_INIT_UNSUPPORTED,GH_PLAYER_INIT_CALLEE} gh_player_init_status;
/* Required real149738 modelinitializer, nativeerror distinctfromoriginalbool.
 * Original107F58 ignoresoriginalbool; nativeerrors/unknownconsumerstillstop.
 * It mustuse SAMEplayer/typedpartowner andrealMD/texture, no parsed==ready. */
typedef gh_player_init_status (*gh_player_model_initialize)(void *,gh_game_title_player *,gh_boot_work *,
 const gh_bundle_entry *geometry,const gh_bundle_entry *texture,uint32_t texture_mode,int *original_return,uint32_t *frontier);
typedef struct {gh_game_player_factory *factory;gh_boot_work *work;
 gh_game_title_player *current_player; /*borrowactualalready-publishedWorkplayer*/
 gh_player_model_initialize model_initialize_00149738;void *model_context;} gh_player_init_bindings;
typedef struct {uint32_t stage,frontier;gh_game_player_factory *factory;gh_boot_work *work;
 gh_bundle_entry geometry,texture;gh_model_view geometry_scripts;
 uint32_t texture_mode;int model_original_return;uint8_t model_complete;
 uint32_t pending_child_id;} gh_game_player_init;
/* Actualvirtual107F58 Main-player entry. Currentrecoveredprefix reachesreal
 * sharedmodel149738, then originalchild101 factory; neither isfakedready.
 * Requiresfull30FA98 factory first andtypedWorkplayerpublication by31C4E0. */
gh_player_init_status gh_game_player_initialize_00107f58(gh_game_player_init *,const gh_player_init_bindings *);
#ifdef __cplusplus
}
#endif
#endif
