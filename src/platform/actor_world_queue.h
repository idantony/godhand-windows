#ifndef GH_ACTOR_WORLD_QUEUE_H
#define GH_ACTOR_WORLD_QUEUE_H
#include "../core/game_actor_model_draw.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_actor_world_queue gh_actor_world_queue;
typedef enum {GH_ACTOR_WORLD_OK,GH_ACTOR_WORLD_ARGUMENT,GH_ACTOR_WORLD_BOUNDS,
 GH_ACTOR_WORLD_MEMORY,GH_ACTOR_WORLD_UNSUPPORTED} gh_actor_world_status;
typedef struct {gh_actor_draw_command command;size_t packets;} gh_actor_world_group;
typedef struct {const gh_static_gs_vertex *vertices;size_t vertex_count;
 const uint32_t *indices;size_t triangles;uint16_t primitive;
 const gh_stage_material *material;uint64_t tex0;size_t stream,batch;} gh_actor_world_packet;
/* Single original group per append; caller owns the ONE global OT. Geometry
 * captures values and truly converts all original stream/batch vertices.
 * Owned outputs survive palette/node changes; model/cache/source references
 * remain borrowed until same-frame reset/destroy. No GPU/frame acknowledgement. */
gh_actor_world_status gh_actor_world_queue_create(gh_actor_world_queue **);
void gh_actor_world_queue_reset(gh_actor_world_queue *);
void gh_actor_world_queue_destroy(gh_actor_world_queue *);
size_t gh_actor_world_queue_count(const gh_actor_world_queue *);
/* Atomic failure leaves existing groups and output index unchanged. Exact
 * VU frontier is retained. Only matched program2/TFX0 or program30/TFX3 profiles are accepted. */
gh_actor_world_status gh_actor_world_queue_append(gh_actor_world_queue *,const gh_actor_draw_command *,size_t *,gh_static_transform_frontier *);
gh_actor_world_status gh_actor_world_queue_group_at(const gh_actor_world_queue *,size_t,gh_actor_world_group *);
gh_actor_world_status gh_actor_world_queue_packet_at(const gh_actor_world_queue *,size_t,size_t,gh_actor_world_packet *);
#ifdef __cplusplus
}
#endif
#endif
