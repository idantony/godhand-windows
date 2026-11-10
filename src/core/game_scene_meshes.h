#ifndef GH_GAME_SCENE_MESHES_H
#define GH_GAME_SCENE_MESHES_H
#include "game_scene_tables.h"
#include "game_room_pools.h"
#include "game_world_light.h"
#include "../assets/model.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {GH_ROOM_OBJECT_SCROLL_MESH=1};
typedef struct gh_stage_textures gh_stage_textures;
typedef struct gh_stage_material gh_stage_material;
typedef struct {const gh_stage_textures *owner;uint32_t slot;uint64_t initial_tex0;
 uint8_t initial_resolved,tm3_override_required;const gh_stage_material *decoded;
 uint64_t consumer_visits;} gh_scene_mesh_material;
typedef enum {GH_MESH_OK,GH_MESH_ARGUMENT,GH_MESH_BOUNDS,GH_MESH_MEMORY,GH_MESH_BUSY,GH_MESH_UNSUPPORTED} gh_scene_mesh_status;
typedef struct {
 gh_model_view script;gh_model_mesh mesh;uint32_t script_index,mesh_index;
 uint16_t submesh_count;uint32_t node_flags;
} gh_scene_mesh_descriptor;
typedef struct {
 const uint8_t *bytes;size_t size,header_offset,position_offset,uv_offset,color_offset;
 uint16_t vertices;uint8_t material;uint32_t parity_packet_bytes;
} gh_scene_mesh_stream;
typedef struct {
 gh_r207_allocation allocation;uint32_t handle;gh_scene_mesh_descriptor descriptor;
 /* Native representation of original self-relative fields D0/F0. Packet
  * allocations own capacity; semantic selectors above replace DMA pointers.
  * Their raw bytes are not a guest packet view for downstream consumers. */
 uint32_t local_matrix_offset,rotation_matrix_offset;uint8_t factory_ready;
 uint32_t node_handle;void *node;gh_r207_allocation packet_allocation;size_t packet_bytes;
 uint64_t test,tex1,alpha,clamp;uint32_t vif_unpack;
 gh_scene_mesh_material materials[16];gh_r207_allocation object_packet_allocation;
 /* Original signed-ID bits: node8000000A suppresses first transfer and
  * chains table11; object positive10 uses table10 then NOP. */
 uint32_t node_program[2],object_program[2],fog_rgb;uint8_t constructor_complete,render_ready;
 uint8_t prefix_stage;
} gh_scene_mesh_object;
typedef struct {const gh_stage_textures *textures;const gh_game_world_light *light;
 const void *elf;size_t elf_bytes;} gh_scene_mesh_construction_bindings;
typedef struct {
 gh_room_pools *pools;gh_heap *heap;gh_game_scene_tables *tables;gh_boot_work *work;
 gh_bundle scp;gh_scene_mesh_descriptor *descriptors;size_t count;
 gh_scene_mesh_object first;uint32_t frontier;int detail;uint8_t prepared;
 uint8_t *factory_serial;
 gh_scene_mesh_construction_bindings construction;
 gh_scene_mesh_object *remaining_objects;size_t current_index,constructed_count;
} gh_scene_meshes;
gh_scene_mesh_status gh_scene_meshes_object_at(const gh_scene_meshes *,size_t,const gh_scene_mesh_object **);
/* Actual per-record148D30 ->1522C0/151398/151368 TM3 replacement. Must be
 * called at the original draw consumer, not an initialization acknowledgment.
 * Returned decoded GS-domain pixels still need real native GPU upload/draw.
 * Borrow expires at the SAMEmanager rangeforget/cacheclear/destroy. */
gh_scene_mesh_status gh_scene_meshes_material_00148d30(gh_scene_meshes *,size_t object_index,
 unsigned submesh,gh_stage_textures *,const gh_stage_material **);
gh_scene_mesh_status gh_scene_meshes_bind_construction(gh_scene_meshes *,const gh_scene_mesh_construction_bindings *);
/* One-time cold seed for the canonical shared factory byte3C3D18. Resolves
 * actual ELF32 PT_LOAD file/BSS data; does not reset it at a room boundary. */
gh_scene_mesh_status gh_scene_mesh_factory_serial_from_elf(const void *,size_t,uint8_t *);
/* Bounded source views for actual31D658/1552F0/155800, not a VU emulator.
 * Vertex XYZ remains signed int16 VIF data; W is raw control/ADC data.
 * VU transformation/strip emission is not claimed by this view. */
gh_scene_mesh_status gh_scene_mesh_stream_at(const gh_scene_mesh_descriptor *,unsigned,gh_scene_mesh_stream *);
gh_scene_mesh_status gh_scene_mesh_position(const gh_scene_mesh_stream *,size_t,int16_t xyz[3],uint16_t *raw_w);
/* Caller zero-inits owner and retains room bytes/pools/heap until release.
 * Original SCR/mesh loop order and prefix ownership, one shared handle in
 * each original object/scroll registry. current_index/constructed_count let
 * callers distinguish an incomplete object from completed source iterations.
 * factory_serial must borrow the canonical original3C3D18 byte; NULL stops
 * before its30FE68 write, never invents an initial serial. With it, currently
 * proves the same-class31FAE8 cast, allocates the canonical149310 node and
 * 155248 packet capacity, then stops at155678 without construction bindings.
 * Bindings resume the supported unit-scale/static constructor profile, with
 * actual SDK-polynomial Euler reference and tagged deferred TM3 materials.
 * Successful full construction does not acknowledge uploads/VU/GS rendering. */
gh_scene_mesh_status gh_scene_meshes_construct_0031d1b0(gh_scene_meshes *,gh_game_scene_tables *,
 gh_boot_work *,const gh_bundle *,gh_room_pools *,gh_heap *);
/* Compatibility helper for the first object only. The whole constructor uses
 * this same finite nearest-binary32 profile for each source iteration.
 * Same canonical texture/light owners, actual ELF constants. All
 * initial material values remain tagged; TM3 replacement is mandatory before
 * consuming them. Caller FEnv is restored. */
gh_scene_mesh_status gh_scene_meshes_finish_first_reference(gh_scene_meshes *,
 const gh_stage_textures *,const gh_game_world_light *,const void *elf,size_t);
/* Releases only the native-owned prefix above. No unknown original virtual
 * destructor is claimed. Full initialized actor teardown remains separate. */
gh_scene_mesh_status gh_scene_meshes_release_prefix(gh_scene_meshes *);
#ifdef __cplusplus
}
#endif
#endif
