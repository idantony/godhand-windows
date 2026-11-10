#ifndef GH_GAME_ACTOR_PACKETS_H
#define GH_GAME_ACTOR_PACKETS_H
#include "game_actor_model.h"
#include "../assets/skinned_mesh.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native semantic DMA/VIF references. Address-bearing guestwords nevercarry
 * hostpointers; rawallocation iscapacity/lifetime, NOT an executableDMAimage. */
typedef struct {gh_skinned_mesh_stream input;size_t first_batch,batches,frame_offset[2];uint32_t material_slot;} gh_actor_packet_stream;
typedef struct {gh_skinned_mesh_batch input;size_t stream,frame_offset[2];} gh_actor_packet_batch;
typedef struct {uint8_t *node;gh_model_view script;size_t mesh_index;gh_bundle_entry texture;
 gh_r207_allocation allocation;gh_heap *heap;size_t bytes,stream_count,batch_count;
 gh_actor_packet_stream streams[16];gh_actor_packet_batch *batches;
 uint64_t test,alpha,tex1,clamp;uint32_t flags,argument,frontier;unsigned detail;
 float box[6];uint8_t bound,ready;} gh_actor_node_packets;
/* Actual148420 dynamicMDB path,155138/154A90/154DA0/154E98 and1490B0.
 * Builds bothparity packetreferences/GSscalarsetup, ownsactualsizedallocation,
 * andcomputesoriginalfloat-position boundingbox. Initialmaterialslot deferred
 * untilrealTM3 drawconsumer; thisdoesNOT acknowledgeGPU/skinning execution.
 * Modeflags must match recovered149738 texturemode|1080 and MDBnum>=2. */
gh_actor_model_status gh_actor_packets_build_00148420(gh_actor_node_packets *,void *canonical_node,size_t,
 const gh_model_view *,size_t mesh_index,const gh_bundle_entry *,uint32_t flags,uint32_t argument,
 gh_heap *packet_heap,gh_static_transform_policy,int *original_return);
gh_actor_model_status gh_actor_packets_release(gh_actor_node_packets *);
#ifdef __cplusplus
}
#endif
#endif
