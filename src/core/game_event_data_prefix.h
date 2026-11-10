#ifndef GH_GAME_EVENT_DATA_PREFIX_H
#define GH_GAME_EVENT_DATA_PREFIX_H

#include "game_event_components.h"
#include "game_event_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t *records;
    size_t resource_offset, bytes, stride;
    uint32_t count;
} gh_event_sequence_view;

typedef gh_game_event_status (*gh_event_effect_data_register)(
    void *context, const uint8_t *bundle, size_t bytes, uint32_t *frontier);

/* Typed host references for the pointer writes in REL+4B88 and +860. The
 * resource and canonical component storage must remain alive during use. */
typedef struct {
    const gh_event_resource *resource;
    const gh_event_components *components;
    const uint8_t *data;
    size_t bytes, camera_table, actor_info_table;
    uint8_t camera_count, actor_count, bound;
    size_t actor_cut_table;
    uint8_t actor_cuts_bound;
    gh_event_sequence_view sequences[GH_EVENT_COMPONENT_COUNT];
    unsigned tail_step;
    uint16_t sequence_mask;
    gh_event_effect_data_register effect_register;
    void *effect_context;
} gh_event_data_prefix;

typedef struct {
    uint32_t flags;
    size_t primary_offset, alternate_offset;
    const uint8_t *primary, *alternate;
} gh_event_camera_cut;

typedef struct {
    size_t primary_offset, sequence_offset;
    const uint8_t *primary, *sequence;
    uint8_t source_cut, timing_cut;
    uint16_t primary_length;
} gh_event_actor_cut_lane;

typedef struct {
    uint32_t flags;
    gh_event_actor_cut_lane lanes[2];
} gh_event_actor_cut;

/* 2977E8 through 297848 only. The original next call 297864 performs
 * additional ActorCut relocation and is separate from this prefix. */
gh_game_event_status gh_event_data_bind_prefix_002977e8(
    gh_event_data_prefix *, gh_event_components *, const gh_event_resource *, uint32_t *frontier);
gh_game_event_status gh_event_camera_cut_at(const gh_event_data_prefix *, size_t, gh_event_camera_cut *);
gh_game_event_status gh_event_camera_cut_length_00004e60(const gh_event_data_prefix *, size_t, uint16_t *);
gh_game_event_status gh_event_actor_info_at(const gh_event_data_prefix *, size_t, const uint8_t **);
/* 297864 -> REL+0870/+0918. Resolves both lanes and verifies all rows before
 * publishing the borrowed table. The next 2977E8 call is Vertex at 297880. */
gh_game_event_status gh_event_actor_cuts_bind_00297864(gh_event_data_prefix *, uint32_t *frontier);
gh_game_event_status gh_event_actor_cut_at(const gh_event_data_prefix *, size_t actor, size_t cut,
                                         gh_event_actor_cut *);
/* Completes the remaining ordered 2977E8 setters. Positive EV01 EffData
 * requires the real registration callback. Retry resumes at the failed leaf. */
gh_game_event_status gh_event_data_bind_tail_00297880(
    gh_event_data_prefix *, gh_event_components *, gh_event_effect_data_register,
    void *context, uint32_t *frontier);
/* Index is the canonical component index from gh_event_component_at. */
gh_game_event_status gh_event_sequence_at(const gh_event_data_prefix *, unsigned component,
                                        gh_event_sequence_view *);

/* Original 297A68 -> REL+1298. Poll the canonical EventActor flag that its
 * create task sets. This leaf never sets the flag or launches the task. */
gh_game_event_status gh_event_actor_creation_poll_00297a68(gh_event_components *, int *ready);
/* REL+12A8 distinguishes normal completion after the readiness poll. */
gh_game_event_status gh_event_actor_creation_end_normal_000012a8(gh_event_components *, int *normal);

#ifdef __cplusplus
}
#endif
#endif
