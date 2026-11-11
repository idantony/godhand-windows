#ifndef GH_GAME_ENEMY_WEAPON_H
#define GH_GAME_ENEMY_WEAPON_H
#include "game_enemy_entry.h"
#include "game_hitarea_objects.h"
#include "game_hitarea_shapes.h"
#include "game_collision_scroll.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {GH_ENEMY_WEAPON_363_BYTES=0x790, GH_ROOM_OBJECT_ENEMY_WEAPON=0x363};
/* Shared original 1C9140, used by both held and room-placed weapons. */
gh_actor_model_status gh_om_weapon_construct_001c9140(void *,size_t,gh_title_player_view *);

typedef struct {
    gh_game_title_player object;
    gh_enemy_entry_bindings bindings;
    gh_game_objects *resource_owner;
    gh_object_view resource;
    gh_object_resource_load load;
    uint32_t handle, frontier;
    uint8_t stage;
} gh_enemy_weapon;

/* Original 30FA98(363,FFFF) for the weapon's factory and dual publication.
 * A true OK/non-null here owns the object before its separate VT+44 call. */
gh_actor_model_status gh_enemy_weapon_create_0030fa98(gh_enemy_weapon *,
    const gh_enemy_entry_bindings *, int *original_nonnull);
/* Native failure/cancellation only, for the pre-virtual factory owner. */
gh_actor_model_status gh_enemy_weapon_release_factory(gh_enemy_weapon *);

typedef struct {
    gh_enemy_weapon *weapon;
    gh_actor_model model;
    gh_bundle_entry geometry,texture;
    gh_hitarea_shape_bindings shapes;
    gh_hitarea_object hit[2];
    int shape_index[2];
    uint32_t texture_mode,frontier;
    gh_actor_model *parent_model;
    const gh_title_player_view *parent;
    gh_game_scene_tables *actors;
    gh_collision_scroll_mutation *scroll;
    uint8_t stage, hit_owned, attach_stage;
} gh_enemy_weapon_init;

/* Common 1C9268 receiver/attacker sphere setup and scoped ownership. */
gh_actor_model_status gh_om_weapon_spheres_001c9268(gh_actor_model *,gh_title_player_view *,gh_game_init_state *,
    const gh_hitarea_shape_bindings *,gh_hitarea_object[2],int[2],uint8_t *owned,uint8_t *phase,uint32_t *frontier);
gh_actor_model_status gh_om_weapon_spheres_release(gh_hitarea_object[2],uint8_t *owned,uint32_t *frontier);

/* Actual VT42AA98+44, narrowly for factory 363. */
gh_actor_model_status gh_enemy_weapon_initialize_001c9268(gh_enemy_weapon_init *,
    gh_enemy_weapon *, const gh_hitarea_shape_bindings *, int *original_return);
/* Original 26AA30 nonnull transform copy, then the kind363/dep202
 * 26AB20 -> 1CF160 attachment. No raw guest pointer is published. */
gh_actor_model_status gh_enemy_weapon_attach_0026ab20(gh_enemy_weapon_init *,
    const gh_title_player_view *parent, gh_actor_model *parent_model,
    gh_game_scene_tables *, gh_collision_scroll_mutation *);
/* Releases only this Init's shapes/model. Factory release follows separately. */
gh_actor_model_status gh_enemy_weapon_release_init(gh_enemy_weapon_init *);

#ifdef __cplusplus
}
#endif
#endif
