#ifndef GH_GAME_SCENE_DYNAMIC_INTERNAL_H
#define GH_GAME_SCENE_DYNAMIC_INTERNAL_H
#include "game_scene_dynamic.h"
struct gh_scene_dynamic_box {gh_scene_dynamic_owner *owner;gh_r207_allocation allocation;gh_title_player_view *actor;const uint8_t *matrix;struct gh_scene_dynamic_box *next;uint8_t world_ready;};
gh_scene_dynamic_status gh_scene_dynamic_validate_owner(gh_scene_dynamic_owner *);
#endif
