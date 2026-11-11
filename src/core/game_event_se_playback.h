#ifndef GH_GAME_EVENT_SE_PLAYBACK_H
#define GH_GAME_EVENT_SE_PLAYBACK_H

#include "game_event_audio.h"
#include "game_event_data_prefix.h"
#include "game_event_link.h"

#ifdef __cplusplus
extern "C" {
#endif

/* REL+9AA8. Invoke at EventSe's ordered PLAY visit, after EventEnv has
 * published its current cut/frame. The EV01 SE/BGM sequence tables are empty. */
gh_game_event_status gh_event_se_move_play_00009aa8(gh_event_link *,
    const gh_event_data_prefix *,gh_event_audio_channel1 *,uint32_t *frontier,int *relative);

/* REL+9CB8 imports DataEnd, which fades archive 0x81 and resets channel 1. */
gh_game_event_status gh_event_se_quit_play_00009cb8(gh_event_link *,
    gh_event_audio_channel1 *,uint32_t *frontier,int *relative);

#ifdef __cplusplus
}
#endif
#endif
