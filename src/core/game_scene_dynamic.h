#ifndef GH_GAME_SCENE_DYNAMIC_H
#define GH_GAME_SCENE_DYNAMIC_H
#include "game_scene_tables.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_DYNAMIC_OK,GH_DYNAMIC_ARGUMENT,GH_DYNAMIC_MEMORY,GH_DYNAMIC_BUSY,GH_DYNAMIC_UNSUPPORTED} gh_scene_dynamic_status;
struct gh_scene_dynamic_box;
typedef struct {gh_game_scene_tables *tables;gh_heap *heap;struct gh_scene_dynamic_box **allocations;size_t allocation_count,capacity;uint32_t frontier;uint8_t bound;} gh_scene_dynamic_owner;
typedef struct {uint8_t *bytes;size_t size;gh_title_player_view *actor;const uint8_t *matrix;
 const struct gh_scene_dynamic_box *next;} gh_scene_dynamic_box_view;
/* SAME original190/194 live list; no second head/count. Foreignhead rejected.
 * Constructordirty points remain VF0 until original virtualupdate is restored. */
gh_scene_dynamic_status gh_scene_dynamic_bind(gh_scene_dynamic_owner *,gh_game_scene_tables *,gh_heap *);
gh_scene_dynamic_status gh_scene_dynamic_box_create_0012ead8(gh_scene_dynamic_owner *,gh_title_player_view *,const uint8_t *matrix,
 const float offset[3],const float rotation_offset[3],const float size[3],const uint8_t attributes[96],
 struct gh_scene_dynamic_box **out,int *original_nonnull);
gh_scene_dynamic_status gh_scene_dynamic_box_view_at(const gh_scene_dynamic_owner *,const struct gh_scene_dynamic_box *,gh_scene_dynamic_box_view *);
/* Native constructor/shutdown abort only, never fake originalvirtualupdate or
 * querycompletion. Unlinks/frees ONLYowner's records, validatesbeforemutation. */
gh_scene_dynamic_status gh_scene_dynamic_box_abort(gh_scene_dynamic_owner *,struct gh_scene_dynamic_box *);
gh_scene_dynamic_status gh_scene_dynamic_release(gh_scene_dynamic_owner *);
#ifdef __cplusplus
}
#endif
#endif
