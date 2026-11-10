#ifndef GH_GAME_OM_DOOR_GEOMETRY_H
#define GH_GAME_OM_DOOR_GEOMETRY_H
#include "game_om_factory.h"
#include "game_scene_dynamic.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Persistent canonical429290 mutable .data attributes; loadonceatboot, same
 * bytesborrowed byall doors/rooms. Never assume staticELFmeansconstmemory. */
typedef struct {uint8_t attributes[96];uint8_t initialized;uint8_t ranges307[12],ranges307_initialized;} gh_om_door_shared;
typedef gh_om_door_shared gh_om_shared_tables;
gh_scene_placement_status gh_om_door_shared_from_elf(gh_om_door_shared *,const void *,size_t);
typedef struct {struct gh_scene_dynamic_box *box;gh_om_object *object;gh_scene_dynamic_owner *owner;uint32_t frontier;uint8_t complete;} gh_om_door_geometry;
/* Actual1BD228: size610+.1XYZ, centeredleft/halfheight offset,zeroEuler,
 * sixcanonicalattrs then real12EAD8/12D970. Original644 istypedbox above. */
gh_scene_placement_status gh_om_door_geometry_001bd228(gh_om_door_geometry *,gh_om_object *,gh_scene_dynamic_owner *,const gh_om_door_shared *,gh_static_transform_policy);
gh_scene_placement_status gh_om_door_geometry_abort(gh_om_door_geometry *);
#ifdef __cplusplus
}
#endif
#endif
