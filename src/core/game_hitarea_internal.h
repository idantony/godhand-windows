#ifndef GH_GAME_HITAREA_INTERNAL_H
#define GH_GAME_HITAREA_INTERNAL_H
#include "game_collision_pair_shapes.h"
typedef struct {gh_game_damage_link link;gh_hitarea_object object;
 gh_collision_pair_shape shape;uint8_t *record;uint8_t associated;} gh_native_hitarea_node;
struct gh_hitarea_native {gh_game_init_state *owner;gh_title_player_view *actors[416];uint32_t serials[416];gh_native_hitarea_node nodes[672];gh_title_player_view *last_hit_actors[416];};
gh_init_status gh_hitarea_clear_record_bytes(uint8_t *,gh_game_damage_link **,unsigned);
#endif
