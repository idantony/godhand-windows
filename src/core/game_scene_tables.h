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
} gh_scene_ray_state;
typedef struct {gh_scene_table_view environment,scroll_texture;
 uint32_t scroll_pairs[32][2],scroll_words[32],scroll_link_words[32];
 gh_bundle_entry akt,akg;gh_scene_actor_link actor_links[8];
 uint32_t actor_vec30[3],actor_vec40[3],actor_vec50[3],actor_count60,actor_count64,actor_count68;
 void *pending_actor_head;uint32_t pending_actor_count;uint8_t actors_ready;
 uint32_t frontier;int detail;gh_scene_ray_state ray;} gh_game_scene_tables;
typedef enum {GH_TABLE_OK,GH_TABLE_ARGUMENT,GH_TABLE_BOUNDS,GH_TABLE_UNSUPPORTED} gh_scene_table_status;
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
