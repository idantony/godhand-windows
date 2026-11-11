#ifndef GH_GAME_SCENE_TABLES_H
#define GH_GAME_SCENE_TABLES_H
#include "boot.h"
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Canonical Work608/scroll7558A8/actor5CAE50/link747B20 companionfields.
 * Originalrelative/guestpointerwords are never modified into hostaddresses. */
typedef struct {const uint8_t *bytes;size_t size;} gh_scene_table_view;
typedef struct {uint32_t relative_word;gh_scene_table_view view;} gh_scene_actor_link;
/* Observed query scratch of the SAME5CAE50 owner. Only XYZ lanes are
 * represented; unknown/preserved vector W lanes are not manufactured. The
 * native reports pointer is never truncated to the original32-bit field. */
typedef struct {
 uint32_t bounds_min90[3],bounds_maxa0[3],fromb0[3],toc0[3],directiond0[3],frome0[3],tof0[3];
 uint32_t report_limit140;void *reports18c;
 uint32_t mask174,flags178,triangle17c,distance184,distance188;
 uint8_t hit_kind180,mode181;
 uint64_t native_queries;
 /* Canonical query fields and original triangle-visited bit4 companion.
  * Bit2 remains in the SAME mutable actor_links[4] flags view. */
 uint32_t grid_min6c[3],grid_max78[3],hit_position120[3],hit_normal130[3];
 struct gh_ray_triangle_state *triangle_state;
 /* Original1321D8 sphere scratch in SAME query owner, not a second manager. */
 uint32_t sphere_center100[3],sphere_radius110;
} gh_scene_ray_state;
struct gh_scene_actor_scroll_state;
struct gh_scene_dynamic_owner;
typedef struct gh_game_scene_tables {gh_scene_table_view environment,scroll_texture;
 uint32_t scroll_pairs[32][2],scroll_words[32],scroll_link_words[32];
 gh_bundle_entry akt,akg;gh_scene_actor_link actor_links[8];
 uint32_t actor_vec30[3],actor_vec40[3],actor_vec50[3],actor_count60,actor_count64,actor_count68;
 /* Legacyhost names: original190/194 LIVE dynamic collision shape list,
  * destroyed by12ECF0 during actor reset; not a deferred-delete queue. */
 void *pending_actor_head;uint32_t pending_actor_count;uint8_t actors_ready;
 uint32_t frontier;int detail;gh_scene_ray_state ray;struct gh_scene_actor_scroll_state *actor_scroll_native;
 struct gh_scene_dynamic_owner *dynamic_native_owner; /*borrow ONE original190/194 typedowner*/
} gh_game_scene_tables;
typedef enum {GH_TABLE_OK,GH_TABLE_ARGUMENT,GH_TABLE_BOUNDS,GH_TABLE_UNSUPPORTED,GH_TABLE_MEMORY} gh_scene_table_status;
/* Work608 is a canonical typed environment view plus native opaque handle1,
 * never a truncated host pointer. 2A71B0 only binds SearchData("ENV",0);
 * actual texture/lighting consumption belongs to later operations.
 * SCP present construction is still a precise unsupported boundary. */
enum {GH_SCENE_ENVIRONMENT_NATIVE_HANDLE=1};
gh_scene_table_status gh_scene_environment_002a71b0(gh_game_scene_tables *,gh_boot_work *,const gh_bundle *);
gh_scene_table_status gh_scene_scrolls_0031d1b0(gh_game_scene_tables *,gh_boot_work *,const gh_bundle *);
/* ActualAKT/AKG init: eightoffsets resolvewithincontainingAKT, including
 * FFFFFFFF=>header-1 when stillinsideparent. Sourceimage remainsimmutable.
 * Nonemptyactor deletionlist explicit12ECF0 frontier, neverfakedestruction. */
/* Actual12E800 clears only AKG and eight links, preserving counts/vectors,
 * then12ECF0 destroys the real pending list (positive list stillunsupported). */
gh_scene_table_status gh_scene_actors_reset_0012e800(gh_game_scene_tables *);
gh_scene_table_status gh_scene_actors_0012e670(gh_game_scene_tables *,const gh_bundle *);
/* ActualR020AKG+2Acount0 path plus308D40 clear32words. */
gh_scene_table_status gh_scene_actor_scroll_links_00307a60(gh_game_scene_tables *);
#ifdef __cplusplus
}
#endif
#endif
