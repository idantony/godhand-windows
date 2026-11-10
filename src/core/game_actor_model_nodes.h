#ifndef GH_GAME_ACTOR_MODEL_NODES_H
#define GH_GAME_ACTOR_MODEL_NODES_H
#include "game_actor_packets.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {size_t header_offset[2],constants_offset[2],palette_offset[2],part_table_offset,bytes;
 uint32_t object_program[2],node_selector[2],palette_unpack,fog_rgb;
 uint64_t dma_constants,cycle,gif,gs_fog_register,dma_palette,ret;
 uint32_t flush,direct,mscnt;const gh_actor_part *parts;size_t part_count;uint8_t ready;
} gh_actor_object_packets;
gh_actor_model_status gh_actor_model_object_packet(const gh_actor_model *,const gh_actor_object_packets **);
/* SAME object packet allocation; returned span is borrowed until model release.
 * Does not allocate, resolve guest pointers, or acknowledge DMA execution. */
gh_actor_model_status gh_actor_model_object_span(gh_actor_model *,size_t offset,size_t bytes,uint8_t **);
/* Same node program-ref owner. HIGH80000000 suppresses the MPG upload,
 * independently of the low8 program identity. This never executes a kernel. */
gh_actor_model_status gh_actor_model_node_program_get(const gh_actor_model *,unsigned,unsigned parity,uint32_t *);
gh_actor_model_status gh_actor_model_node_program_set(gh_actor_model *,unsigned,unsigned parity,uint32_t);
gh_actor_model_status gh_actor_model_object_program_reset(gh_actor_model *,unsigned parity);
/* Narrow originalconsumer flag writes, onlynode380/410, SAMEreadyowner. */
gh_actor_model_status gh_actor_model_node_or_flags(gh_actor_model *,unsigned index,uint16_t offset,uint32_t value);
/* Same bounded node owner, for original visibility bit set/clear consumers. */
gh_actor_model_status gh_actor_model_node_mask_flags(gh_actor_model *,unsigned index,uint16_t offset,uint32_t and_mask,uint32_t or_mask);
/* Shared149738 continuation. Public callers use initialize, not a second init. */
gh_actor_model_status gh_actor_model_build_nodes(gh_actor_model *,gh_static_transform_policy,int *);
gh_actor_model_status gh_actor_model_release_nodes(gh_actor_model *);
gh_actor_model_status gh_actor_model_packet_at(const gh_actor_model *,unsigned,const gh_actor_node_packets **);
#ifdef __cplusplus
}
#endif
#endif
