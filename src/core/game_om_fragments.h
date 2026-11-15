#ifndef GH_GAME_OM_FRAGMENTS_H
#define GH_GAME_OM_FRAGMENTS_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t kind, bytes, creator, vtable, destructor, move;
    uint32_t box_y, box_extent[3];
    uint8_t fragments, receiver_flag;
} gh_om_fragment_profile;

/* 031B, 0336 and 0337 share the original 1C27E8/1C28A0/1C29D0 class. */
const gh_om_fragment_profile *gh_om_fragment_profile_for(uint32_t kind);
gh_scene_placement_status gh_om_fragment_construct(const gh_om_fragment_profile *,
    void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om_fragment_initialize(const gh_om_fragment_profile *,
    gh_om303_state *,gh_om_object *,const gh_om_initializer_bindings *,int *);

/* Actualshared1C28A0: half610=30,layer0visible/40hidden, signed60C
 * countselectsdirectpart1..N-1 andsetsoriginal154bits8/10. */
gh_scene_placement_status gh_om_fragments_initialize_001c28a0(gh_om_init_state *);
#ifdef __cplusplus
}
#endif
#endif
