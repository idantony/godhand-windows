#ifndef GH_GAME_SCENE_DYNAMIC_H
#define GH_GAME_SCENE_DYNAMIC_H
#include "game_scene_tables.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_DYNAMIC_OK,GH_DYNAMIC_ARGUMENT,GH_DYNAMIC_MEMORY,GH_DYNAMIC_BUSY,GH_DYNAMIC_UNSUPPORTED} gh_scene_dynamic_status;
struct gh_scene_dynamic_box;
struct gh_dynamic_corner_tables;
typedef struct gh_scene_dynamic_owner {gh_game_scene_tables *tables;gh_heap *heap;struct gh_scene_dynamic_box **allocations;size_t allocation_count,capacity;uint32_t frontier;uint8_t bound;const void *identity;
 const struct gh_dynamic_corner_tables *world_tables; /*borrowONEsource globals for geometry/ray*/
 struct gh_scene_dynamic_box *update_cursor;const struct gh_dynamic_corner_tables *update_tables;const gh_boot_work *update_work;
 size_t update_completed,update_allocation_count;uint8_t update_active;
} gh_scene_dynamic_owner;
typedef struct {uint8_t *bytes;size_t size;gh_title_player_view *actor;const uint8_t *matrix;
 const struct gh_scene_dynamic_box *next;uint8_t world_ready;} gh_scene_dynamic_box_view;
/* SAME original190/194 live list; no second head/count. Foreignhead rejected.
 * Constructor points remain VF0 until original12DCB8 box/12D1A8 quad update. Flag4 selects
 * externalmatrix mode; it is NOT a dirty bit. world_ready is hostmetadata. */
gh_scene_dynamic_status gh_scene_dynamic_bind(gh_scene_dynamic_owner *,gh_game_scene_tables *,gh_heap *);
gh_scene_dynamic_status gh_scene_dynamic_box_create_0012ead8(gh_scene_dynamic_owner *,gh_title_player_view *,const uint8_t *matrix,
 const float offset[3],const float rotation_offset[3],const float size[3],const uint8_t attributes[96],
 struct gh_scene_dynamic_box **out,int *original_nonnull);
/* Actual12E990 ->12CF70 quad, allocated100bytes in the SAME190/194 list. */
gh_scene_dynamic_status gh_scene_dynamic_quad_create_0012e990(gh_scene_dynamic_owner *,gh_title_player_view *,const uint8_t *matrix,
 const float offset[3],const float rotation_offset[3],float width,float height,const uint8_t attributes[16],
 struct gh_scene_dynamic_box **out,int *original_nonnull);
gh_scene_dynamic_status gh_scene_dynamic_box_view_at(const gh_scene_dynamic_owner *,const struct gh_scene_dynamic_box *,gh_scene_dynamic_box_view *);
/* Native constructor/shutdown abort only, never fake originalvirtualupdate or
 * querycompletion. Unlinks/frees ONLYowner's records, validatesbeforemutation. */
gh_scene_dynamic_status gh_scene_dynamic_box_abort(gh_scene_dynamic_owner *,struct gh_scene_dynamic_box *);
/* Original1311A0 unique sourceactor list at+34[8], count remains original+54.
 * Borrowed views must be detached before their actor storage is released.
 * Detach clears typed references only; original count/order are preserved. */
gh_scene_dynamic_status gh_scene_dynamic_sphere_contact_001311a0(gh_scene_dynamic_owner *,struct gh_scene_dynamic_box *,const gh_title_player_view *,int *inserted);
gh_scene_dynamic_status gh_scene_dynamic_sphere_contact_at(const gh_scene_dynamic_owner *,const struct gh_scene_dynamic_box *,unsigned,const gh_title_player_view **);
/* Actual1BD430 callsite only: clear original34[8]/54 AND these SAME
 * typed contacts after the caller has consumed them. Not a frame reset. */
gh_scene_dynamic_status gh_scene_dynamic_contacts_clear_001bd430(gh_scene_dynamic_owner *,struct gh_scene_dynamic_box *);
void gh_scene_dynamic_forget_sphere_actor(gh_scene_dynamic_owner *,const gh_title_player_view *);
/* Host frame cancellation only, before native abort/whole shutdown; not an
 * original update or destructor acknowledgment. */
void gh_scene_dynamic_cancel_update(gh_scene_dynamic_owner *);

gh_scene_dynamic_status gh_scene_dynamic_release(gh_scene_dynamic_owner *);
#ifdef __cplusplus
}
#endif
#endif
