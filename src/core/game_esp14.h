#ifndef GH_GAME_ESP14_H
#define GH_GAME_ESP14_H

#include "game_actor_model.h"
#include "game_actor_motion.h"
#include "game_effect_node.h"
#include "game_objects.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { GH_ESP14_VTABLE = 0x44ee38, GH_ESP_MODEL_KIND = 0x500 };

typedef struct {
    gh_room_pools *pools;
    gh_boot_work *work;
    uint8_t *factory_serial;
    gh_heap *packet_heap;
    const void *fog_rgb;
    gh_static_transform_policy policy;
    const float *canonical_rate;
} gh_esp14_model_bindings;

/* Stable owner of the kind500 child. Register and resolve this same object
 * through 754C30/754C58, including during ordinary scene draw. */
typedef struct {
    gh_game_title_player object;
    gh_actor_model model;
    gh_room_pools *pools;
    uint32_t handle;
    gh_actor_motion motion;
    gh_effect_view motion_source;
    size_t motion_offset;
    gh_boot_work *motion_work;
    const float *motion_rate;
    uint8_t virtual_initialized, model_destroyed, motion_step_applied;
} gh_esp14_model;

typedef struct {
    gh_bundle_entry geometry, texture;
    gh_effect_view motions;
} gh_esp14_resources;

/* 2FFD88 tail after the allocator has run the shared 2FBCC8 base. */
void gh_esp14_construct_tail_002ffd88(uint8_t payload[0x400]);

/* 2F1D58 prefix before 2F1B60. parameter_40 is the caller's actual word,
 * independent of the record. It is not a host pointer. */
gh_effect_status gh_esp14_init_prefix_002f1d58(uint8_t payload[0x400],
    gh_effect_view record, uint32_t parameter_40, uint32_t *frontier);

/* Original global 741DC8 is the effect owner's shared 64 entry EFMs. */
gh_effect_status gh_esp14_resource_00306c78(const gh_game_effects *,
    unsigned index, gh_esp14_resources *, int *original_found);

gh_effect_status gh_esp14_model_create_0030fa98(gh_esp14_model *,
    const gh_esp14_model_bindings *, int *original_found, uint32_t *frontier);

/* 2B8820 -> 149738 for the bounded original default-texture mode 0.
 * Success means only the child model initialized. 2F1D58 still owns the
 * subsequent pose, packet state, hierarchy and virtual return. */
gh_effect_status gh_esp14_model_initialize_002f1b60(gh_esp14_model *,
    const gh_esp14_model_bindings *, const gh_esp14_resources *,
    int *original_return, uint32_t *frontier);

/* Native shutdown of the partial/ready child, distinct from 312708. */
gh_effect_status gh_esp14_model_dispose(gh_esp14_model *);

/* Shared 300EF8 positive record flag 0x00200000. Inputs use the same staged
 * slot as the caller. It consumes the original three LCG steps. */
gh_effect_status gh_effect_position_init_scaled_00301c20(uint8_t[0x400],gh_effect_view,int,float,float[3],uint32_t*);
gh_effect_status gh_esp14_position_init_00301c20(uint8_t staged_payload[0x400],
    gh_effect_view, int mirrored, float position[3], uint32_t *frontier);
typedef struct {
    gh_effect_create_request request;
    uint8_t group_74;
    uint32_t config_40, config_48;
} gh_esp14_nested_request;

typedef gh_effect_status (*gh_esp14_nested_create)(void *,
    const gh_esp14_nested_request *, const gh_actor_model *,
    int *original_return, uint32_t *frontier);

typedef struct {
    gh_esp14_model_bindings model;
    gh_game_effects *effects;
    const float *atan_coeff;
    gh_esp14_nested_create nested_create;
    void *context;
} gh_esp14_bindings;

typedef struct gh_esp14_instance {
    gh_esp14_model child;
    gh_esp14_resources resources;
    gh_game_effects *effects;
    unsigned slot;
    uint32_t serial;
    uint8_t stage, complete, child_delete_requested, esp_destroyed;
} gh_esp14_instance;

/* 2F1D58 full default-texture, unit-UV-basis domain. The stable slot-owned
 * instance outlives both the ESP and registry registrations. The callback
 * retains its own original306528 continuation on failure. */
gh_effect_status gh_esp14_initialize_002f1d58(gh_esp14_instance *,
    const gh_esp14_bindings *, const gh_effect_esp_view *, unsigned slot,
    uint32_t source_parameter_40, int *original_return, uint32_t *frontier);

/* Normal 30F090 kind500 virtual Move. The next 30F650 normal virtual Trans
 * is generic actor rendering at 14A3E8; its canonical model is child.model. */
gh_effect_status gh_esp14_model_move_002b8850(gh_esp14_model *, const gh_boot_work *,
    gh_static_transform_policy, uint32_t *frontier);

/* Actual 2FD610 oscillating local-position branch for this row class. */
gh_effect_status gh_esp14_position_move_002fd610(gh_game_effects *, unsigned slot,
    const gh_effect_esp_view *, int scaled, float factor,
    gh_static_transform_policy, uint32_t *frontier);

/* 2F2250 suffix after the shared 2FBED0 call and bitmap test 2FB060. */
gh_effect_status gh_esp14_move_after_base_002f2250(gh_esp14_instance *,
    const gh_esp14_bindings *, const gh_effect_esp_view *, int active,
    uint32_t *frontier);
/* The ESP's own Trans emits no packet. It conditionally propagates two
 * visibility bits to the ordinary model object using the typed +114 owner. */
gh_effect_status gh_esp14_trans_002f2758(gh_esp14_instance *,
    const gh_effect_esp_view *, uint32_t *frontier);

/* Scope of the real nested 2F1D3C group1 emitter/particle and its typed
 * kind500 attachment. No raw EE pointer can authenticate this domain. */
int gh_esp14_nested_sprite_is_bound(gh_effect_view, const gh_title_player_view *,
    const gh_title_player_view *);

/* Actual 302BB4..302C74 for the original nested zero-config-pose branch.
 * Actor+80 is the live root transform selected by emitter+104 (-2). */
gh_effect_status gh_esp14_nested_emitter_pose_00302bb4(gh_game_effects *,
    unsigned emitter_slot, const gh_effect_emitter_view *, const float atan_coeff[8],
    gh_static_transform_policy, uint32_t *frontier);

/* True 2FEC38 final alpha branch. The attached actor path replaces the
 * ordinary sprite fade with its own tint / three byte opacity factors. */
gh_effect_status gh_effect_actor_sprite_alpha_002fec38(const gh_game_effects *,const gh_effect_esp_view *,
 float alpha,float fade,float *out,uint32_t *frontier);
gh_effect_status gh_esp14_nested_sprite_alpha_002fec38(const gh_effect_esp_view *,
    float alpha, float fade, float *out, uint32_t *frontier);

typedef gh_effect_status (*gh_esp14_sound_detach)(void *, gh_actor_model *, uint32_t *frontier);

/* Actual hidden-pool virtual destructor 2F27E8(flags=2). The original
 * caller still clears the active bit after success, as for other classes. */
gh_effect_status gh_esp14_destroy_hidden_002f27e8(gh_esp14_instance *,
    gh_esp14_sound_detach, void *context, uint32_t *frontier);

/* Original kind500 virtual destructor (flags=3) at the scene owner's
 * deletion point. The scene caller still removes the canonical handle.
 * The native arena bytes may be collected separately after FX borrows end. */
gh_effect_status gh_esp14_child_destroy_002b8908(gh_esp14_instance *,
    const gh_game_objects *room, const gh_game_objects *global, uint32_t *frontier);

/* Host-only storage collection after both original destructor calls. A live
 * FX borrow yields collected=0 without pausing the scheduler. */
gh_effect_status gh_esp14_collect_retired(gh_esp14_instance *, int *collected,
    uint32_t *frontier);
/* The room owns the stable registered child and its draw continuation.
 * Effects supply the current original slot identity at each virtual call. */
typedef struct gh_esp14_provider {
    void *context;
    gh_effect_status (*initialize)(void *, const gh_effect_esp_view *, unsigned slot,
        uint32_t source_parameter_40, int *original_return, uint32_t *frontier);
    gh_effect_status (*move)(void *, const gh_effect_esp_view *, unsigned slot,
        int active, uint32_t *frontier);
    gh_effect_status (*trans)(void *, const gh_effect_esp_view *, unsigned slot,
        uint32_t *frontier);
    gh_effect_status (*destroy_hidden)(void *, const gh_effect_esp_view *, unsigned slot,
        uint32_t *frontier);
    gh_effect_status (*collect_retired)(void *, uint32_t *frontier);
} gh_esp14_provider;
#ifdef __cplusplus
}
#endif
#endif
