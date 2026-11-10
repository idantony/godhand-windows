#ifndef GH_GAME_PLAYER_ABORT_H
#define GH_GAME_PLAYER_ABORT_H
#include "game_title_player.h"
#include "sound_control.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_ABORT_OK,GH_PLAYER_ABORT_ARGUMENT,GH_PLAYER_ABORT_BUSY,GH_PLAYER_ABORT_UNSUPPORTED,GH_PLAYER_ABORT_CALLEE} gh_player_abort_status;
/* Required positive-handle consumers. Values are opaque original/native IDs,
 * never host addresses or executable guest pointers. OK must mean real action
 * setExit or COMPLETE cSnd_SeStop wrapper (including its stop-slot bookkeeping),
 * not only parsing or queueing unrelated audio. Zero handles use verified
 * original early-return paths and do not invoke these callbacks. */
typedef gh_player_abort_status (*gh_player_abort_consumer)(void *,uint32_t,uint32_t *frontier);
typedef struct {gh_game_title_player *player;gh_sound_control *sound;
 gh_player_abort_consumer action_exit_001f6f10,se_stop_002cbba0;void *context;} gh_player_abort_bindings;
typedef struct {uint8_t *player_bytes;gh_sound_control *sound;uint32_t stage,pending_handle,frontier;
 uint8_t active,pending;} gh_game_player_abort;
/* Actual126770 action exit ->SeStop161C ->clearSoundB0/AC400000 ->SeStop1620.
 * Failure retains stage and original callback argument across retries. Fields
 * are cleared ONLY after the corresponding actual callee finishes. A completed
 * call resets metadata for the next original abort invocation; owns no actor,
 * sound/device/voice/task/prompt and creates no shadow global state. */
gh_player_abort_status gh_game_player_abort_00126770(gh_game_player_abort *,const gh_player_abort_bindings *);
#ifdef __cplusplus
}
#endif
#endif
