#ifndef GH_GAME_SCENE_DYNAMIC_INTERNAL_H
#define GH_GAME_SCENE_DYNAMIC_INTERNAL_H
#include "game_scene_dynamic.h"
struct gh_scene_dynamic_box {gh_scene_dynamic_owner *owner;gh_r207_allocation allocation;gh_title_player_view *actor;const uint8_t *matrix;struct gh_scene_dynamic_box *next;uint8_t world_ready;const gh_title_player_view *sphere_contacts[8];uint8_t sphere_contact_known[8];size_t original_bytes;struct gh_scene_dynamic_box **borrowed_slot;};
gh_scene_dynamic_status gh_scene_dynamic_validate_owner(gh_scene_dynamic_owner *);
#endif
