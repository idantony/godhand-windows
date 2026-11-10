#ifndef GH_GAME_ACTION_BUTTON_SET_H
#define GH_GAME_ACTION_BUTTON_SET_H
#include "game_action_buttons.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_ACTION_SET_OK,GH_ACTION_SET_ARGUMENT,GH_ACTION_SET_BOUNDS,GH_ACTION_SET_UNSUPPORTED} gh_action_set_status;
typedef struct {int32_t priority,life,unknown20;uint32_t callback_id;void *argument;int32_t kind;} gh_action_set_request;
/* Actualvoid1F7858: first8entry flags&201!=1, exact7scalar/opaqueidentity
 * writes then1F7800 original headinsert. Fullpool=OK/emitted0 (not error).
 * Callback ID is only identity, never cast/invoked by this publisher. */
gh_action_set_status gh_action_button_set_001f7858(gh_game_action_buttons *,const gh_action_set_request *,
 int *emitted,unsigned *entry_index,uint32_t *frontier);
/* Original list starts at header15, and both kinds' +4 are nextlinks. */
gh_action_set_status gh_action_button_first(const gh_game_action_buttons *,gh_action_link *);
gh_action_set_status gh_action_button_next(const gh_game_action_buttons *,gh_action_link,gh_action_link *);
gh_action_set_status gh_action_button_prompt(const gh_game_action_buttons *,gh_action_link,const gh_action_prompt **);
#ifdef __cplusplus
}
#endif
#endif
