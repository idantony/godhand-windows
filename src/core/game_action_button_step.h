#ifndef GH_GAME_ACTION_BUTTON_STEP_H
#define GH_GAME_ACTION_BUTTON_STEP_H
#include "game_action_button_set.h"
#include "game_cockpit.h"
#include "game_init.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_ACTION_STEP_OK,GH_ACTION_STEP_ARGUMENT,GH_ACTION_STEP_BOUNDS,GH_ACTION_STEP_UNSUPPORTED,GH_ACTION_STEP_CALLEE} gh_action_step_status;
typedef gh_action_step_status (*gh_action_callback)(void *,uint32_t original_entry,void *argument,uint32_t *frontier);
typedef struct {gh_game_action_buttons *buttons;gh_boot_work *work;gh_game_cockpit *cockpit;
 const gh_title_player_view *current_player;gh_game_init_state *init;
 const gh_title_player_view *aux_6a0;uint8_t auxiliary_binding_known;
 gh_action_callback callback;void *context;} gh_action_step_bindings;
typedef struct {const void *identity;const gh_action_step_bindings *bindings;uint32_t stage,visited,selected,cleanup_index,frontier,callback_entry;int32_t callback_kind;void *callback_argument;
 gh_action_link link;uint8_t active,complete,pressed;} gh_action_button_frame;
gh_action_step_status gh_action_button_frame_begin(gh_action_button_frame *);
/* True1F7360: mixedgraph walk, eligibility, SAMEHUD write evenwithoutinput,
 * exacthighword1A4 buttons, requiredcallback, then8entry orderedcleanup.
 * A failedcallback preserves prefix andmustretry SAMEframecompanion. */
gh_action_step_status gh_action_button_step_001f7360(gh_action_button_frame *,const gh_action_step_bindings *);
#ifdef __cplusplus
}
#endif
#endif
