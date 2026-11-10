#ifndef GH_STATIC_MODEL_CONSTANTS_H
#define GH_STATIC_MODEL_CONSTANTS_H
#include "static_mesh_transform.h"
#include "../core/game_camera.h"
#include "../core/game_render_projection.h"
#include "../core/game_world_light.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native per-draw counterparts of original objectpacket+10/+50/+90/+110.
 * They are derived values for one SAME canonical object, not another camera. */
typedef struct {float screen[16],clip[16],identity[16],projection[16];} gh_static_model_constants;
/* Actual normal14D0E8; borrows the actual canonical staticobject+80, active
 * PlCamera+0 and shared1538F0 projection owner. Reflection80000 is precise
 * unsupported; mesh.info200000 legitimately bypasses object multiplication. */
gh_static_mesh_status gh_static_model_constants_0014d0e8(const gh_scene_mesh_object *,
 const gh_game_pl_camera *,const gh_game_render_projection *,gh_static_transform_policy,
 gh_static_model_constants *,uint32_t *frontier);
typedef struct {gh_static_transform_inputs inputs;float projection[16];uint32_t fog_rgb;uint8_t fog_written;} gh_static_node_packet;
/* Actual153B00 normal staticcolor/fog path feeding layer14. No matrix/default
 * tint is synthesized. fog_written tells the caller when the canonical
 * object packet fog_rgb must be updated; skip-fog branches preserve it. Unknown dynamiclighting/specialmesh/node modes remain
 * precise frontiers. Output is the existing transform API's requiredinput. */
gh_static_mesh_status gh_static_node_constants_00153b00(const gh_scene_mesh_object *,
 const gh_boot_work *,const gh_game_world_light *,const gh_static_model_constants *,
 gh_static_transform_policy,gh_static_node_packet *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
