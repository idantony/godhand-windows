#ifndef GH_GAME_ACTOR_FINISH_MODEL_H
#define GH_GAME_ACTOR_FINISH_MODEL_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual14A038: root20 gate, optionalroot147A48, then orderedpart1501D0.
 * Preserves solved LOCAL matrices; doesnot rerun149EB8/buildlocalEuler.
 * SAME externalparent typedborrow applies tochildpart0. Positiveblend performs
 *147568(parentworld),30A2E0 scratch,then156D70(CURRENT LOCAL0,old40) using
 *same model.blend_globals; it never consumes scratch as LOCAL. root40004
 *special andpart800 retain originalfrontiers.
 * Nativebinary32reference, callerFEnv preserved; prefixwrites retained. */
gh_actor_model_status gh_actor_finish_model_0014a038(gh_actor_model *,gh_static_transform_policy);
/* Optional continuation of ONE invocation. It owns no model or part storage.
 * Keep after a frontier; clear for a new original call. Do not reset the model
 * or release its owners while this cursor is live. Original one-shot API
 * above remains compatible but must not be retried after partial writes. */
typedef struct {gh_actor_model *model;const uint8_t *root_bytes;
 uint32_t root_flags,part_flags;unsigned part_index,part_count;uint8_t stage;
} gh_actor_finish_model_call;
gh_actor_model_status gh_actor_finish_model_resume_0014a038(gh_actor_finish_model_call *,gh_actor_model *,gh_static_transform_policy);
#ifdef __cplusplus
}
#endif
#endif
