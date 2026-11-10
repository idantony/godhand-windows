#ifndef GH_STATIC_WORLD_QUEUE_H
#define GH_STATIC_WORLD_QUEUE_H
#include "../core/game_object_step.h"
#include "../assets/static_mesh_program10_packets.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_static_world_queue gh_static_world_queue;
typedef enum {GH_WORLD_OK,GH_WORLD_ARGUMENT,GH_WORLD_BOUNDS,GH_WORLD_MEMORY,GH_WORLD_UNSUPPORTED} gh_static_world_status;
typedef struct {gh_object_static_command command;size_t packets;uint32_t captured_fog_rgb;} gh_static_world_group;
typedef struct {
 const gh_static_gs_vertex *vertices;size_t vertex_count;
 const uint32_t *indices;size_t triangles;
 uint16_t primitive;uint8_t material_index;
 const gh_stage_material *material;
 size_t substream,batch;
} gh_static_world_packet;
/* One original emit = one group, including setup/zero-coverage geometry.
 * No second OT: append returned index to the caller's SINGLE global OT.
 * Groups are insertion-order only. Program10 packet order remains fans->strip.
 * Copy values, borrow object/source/material until same-frame reset/destroy.
 * Raw postVU XYZ/F remain FTOI4 words: GS PACKED XYZF2 extracts low16 XY,
 * (uint32 Z>>4)&FFFFFF, (fog_fixed4>>4)&FF. This API performs no GPU ack. */
gh_static_world_status gh_static_world_queue_create(gh_static_world_queue **);
void gh_static_world_queue_reset(gh_static_world_queue *);
void gh_static_world_queue_destroy(gh_static_world_queue *);
size_t gh_static_world_queue_count(const gh_static_world_queue *);
/* Atomic failure: existing groups and output index are unchanged. Explicit
 * binary32 reference math, not hardware-bitexact. Frontier retains VU PC. */
gh_static_world_status gh_static_world_queue_append(gh_static_world_queue *,const gh_object_static_command *,size_t *,gh_static_transform_frontier *);
gh_static_world_status gh_static_world_queue_group_at(const gh_static_world_queue *,size_t,gh_static_world_group *);
gh_static_world_status gh_static_world_queue_packet_at(const gh_static_world_queue *,size_t,size_t,gh_static_world_packet *);
#ifdef __cplusplus
}
#endif
#endif

