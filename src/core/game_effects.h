#ifndef GH_GAME_EFFECTS_H
#define GH_GAME_EFFECTS_H
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded immutable resourceview. All relative offsets stay in owned assets;
 * no relocated guest address is ever a native pointer. */
typedef struct {const uint8_t *bytes;size_t size;} gh_effect_view;
typedef struct {int32_t kind;gh_effect_view tbl1,tbl0;uint32_t references;} gh_effect_resource;
typedef struct {int32_t previous,next;uint8_t in_use,flag9;uint16_t flags_a;
 uint32_t vector_start;uint16_t units,field12;uint32_t unknown14;} gh_effect_pool_node;
struct gh_effect_native;
typedef struct {gh_effect_resource resources[64];gh_effect_view emd_tables[1250],emitter_entries[64];
 gh_effect_view ssd,ear;uint32_t cached408,cached40c,field414;uint8_t active;
 uint32_t vectors[1000][4];gh_effect_pool_node nodes[32],head;uint32_t pool_words[32];
 uint8_t ready;uint32_t frontier;int detail;struct gh_effect_native *native;
 uint8_t event_registered_41c;gh_effect_view event_data;} gh_game_effects;
typedef enum {GH_EFFECT_OK,GH_EFFECT_ARGUMENT,GH_EFFECT_BOUNDS,GH_EFFECT_UNSUPPORTED} gh_effect_status;
/* Borrow exactcanonicalbuffers alreadyreset by2FAA20/3003F0. Live slots need
 * realoriginalkillconsumers, currently explicitunsupported302918/2FBE50.
 * Coldzero/inactive/paused/hidden paths execute originalgates, not fakekills.
 * Original305A68 uses assets.core as EffDataHdr offsets14/1C. */
gh_effect_status gh_game_effects_initialize_00305a68(gh_game_effects *,
 uint8_t *esp_buffer,size_t esp_bytes,uint8_t *emitter_buffer,size_t emitter_bytes,
 const void *core_dat,size_t core_bytes,const gh_bundle *room);
/* Actual306D18/306F88 pool initialization: XYZzero, Wpreserved;32nodes plus
 * sentinel use explicitindices (-1=NULL,32=head) instead of32bitguestpointers. */
void gh_game_effects_pool_reset_00306d18(gh_game_effects *);
/* 2FA0B0 -> 305758, original bank 6. The EV01 bundle is borrowed until
 * 2FA0E8/305778 removes its references or the effects owner is retired. */
gh_effect_status gh_game_effects_register_event_002fa0b0(gh_game_effects *, const void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
