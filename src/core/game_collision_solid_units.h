#ifndef GH_GAME_COLLISION_SOLID_UNITS_H
#define GH_GAME_COLLISION_SOLID_UNITS_H
#include "game_collision_solids.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SOLID_UNIT_OK,GH_SOLID_UNIT_ARGUMENT,GH_SOLID_UNIT_MEMORY,GH_SOLID_UNIT_UNSUPPORTED,GH_SOLID_UNIT_BUSY} gh_solid_unit_status;
typedef struct gh_collision_native_sphere {struct gh_collision_native_sphere *next;gh_r207_allocation allocation;
 const uint8_t *matrix;uint32_t radius_bits;uint8_t constructed;} gh_collision_native_sphere;
typedef struct gh_collision_solid_unit {struct gh_collision_solid_unit *next;gh_game_collision_solids *manager;
 gh_title_player_view *object;gh_heap *heap;gh_r207_allocation allocation;
 gh_collision_native_sphere *spheres;uint32_t type,bound_radius_bits;} gh_collision_solid_unit;
/* SAME462FC0 owner only. Original40/80-byte allocations use actualdefaultheap;
 * typed metadata replaces list/object/matrix pointers, preserves unknownbytes. */
gh_solid_unit_status gh_collision_solid_create_unit_001346f8(gh_game_collision_solids *,gh_heap *,gh_title_player_view *,uint32_t type,uint32_t bound_radius_bits,gh_collision_solid_unit **,uint32_t *frontier);
gh_solid_unit_status gh_collision_solid_create_sphere_00134880(gh_game_collision_solids *,gh_title_player_view *,const uint8_t matrix[64],const uint32_t local_xyz[3],uint32_t radius_bits,gh_collision_native_sphere **,uint32_t *frontier);
/* Actual1347E8 unlink→1344A0 sphere dtor/free→unitfree, preserves otherunits.
 * LookupNULL/missingobject is originalno-op. No query/ChkHit successimplied. */
gh_solid_unit_status gh_collision_solid_release_unit_001347e8(gh_game_collision_solids *,gh_title_player_view *,uint32_t *frontier);
/* Must be empty BEFORE existing134690 room reset; never clear livehead. */
int gh_collision_solid_units_empty(const gh_game_collision_solids *);
/* Explicit native shutdown only, not substituted for originalroomdestructors. */
gh_solid_unit_status gh_collision_solid_units_shutdown(gh_game_collision_solids *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
