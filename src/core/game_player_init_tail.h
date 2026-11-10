#ifndef GH_GAME_PLAYER_INIT_TAIL_H
#define GH_GAME_PLAYER_INIT_TAIL_H
#include "game_player_init.h"
#include "game_player_child_attach.h"
#include "game_player_shadow.h"
#include "game_collision_solid_units.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Optional full100378 provider for realsharedmodel/pose/spring integration.
 * Nativeerrors distinctfromoriginalbool. NULL uses strictrecovered100378,
 * currently148420. ReturningOK mustrepresentactualfullmethodcompletion. */
typedef gh_title_player_status (*gh_player_child_initialize)(void *,gh_game_player_child *,int *original_return,uint32_t *frontier);
typedef struct {gh_game_player_init *prefix;gh_actor_model *parent_model;
 gh_game_player_child *child;gh_player_child_attachment *attachment;
 gh_game_collision_solids *solids;gh_heap *default_heap;gh_core_save_view save;
 gh_object_resource_bindings resources;gh_player_child_initialize initialize_child;void *child_context;
} gh_player_init_tail_bindings;
typedef struct {gh_player_init_tail_bindings b;gh_game_title_player *player;
 gh_player_shadow shadow;gh_collision_solid_unit *solid_unit;gh_collision_native_sphere *sphere;
 uint32_t stage,frontier;int child_original_return;uint8_t bound,active_child;
} gh_game_player_init_tail;
/* Continuesactual107F58 onlyaftertheexistingprefix's realmodelcall completed.
 * Samechildfactory/virtualInit/attachment→shadow→partflags→solidUnit/Sphere→HP.
 * Current nextactual1FE218 hitarea constructor remains precisefrontier;
 * no wholeInit success or31C5C0 jump is manufactured. */
gh_player_init_status gh_game_player_initialize_tail_00107f58(gh_game_player_init_tail *,const gh_player_init_tail_bindings *);
#ifdef __cplusplus
}
#endif
#endif
