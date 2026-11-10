#ifndef GH_GAME_EVENT_ENVIRONMENT_H
#define GH_GAME_EVENT_ENVIRONMENT_H
#include "game_event.h"
#include "game_title_player.h"
#include "display_state.h"
#include "screen_fade.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Separate original global586B30, not cEvent.flags at586AF8. Record identity
 * borrows the persistent boot ELF; no guest pointer enters native storage. */
typedef struct {const uint8_t *record;uint32_t record_address,flags;uint8_t initialized;} gh_game_event_config;
gh_game_event_status gh_game_event_config_from_elf(gh_game_event_config *,const void *,size_t);
/* Original 32-bit address arithmetic, then bounded file-backed record access.
 * This verifies the read, not validity of an arbitrary game event number;
 * no guessed table length or fabricated zero row is introduced. */
gh_game_event_status gh_game_event_config_set_00299320(gh_game_event_config *,int32_t,const void *,size_t);
gh_game_event_status gh_game_event_reset_00297660(gh_game_event *);
/* Actual once-only reset/config/dataNo/flag2 order. Native read errors are
 * validated before committing, rather than falsely acknowledging a read. */
gh_game_event_status gh_game_event_init_environment_002975f8(gh_game_event *,gh_game_event_config *,int32_t,const void *,size_t);
typedef struct {gh_game_event *event;gh_game_event_config *config;gh_boot_work *work;
 const gh_title_player_view *player;gh_screen_fade *fade;const gh_display_state *display;
 const void *elf;size_t elf_bytes;} gh_event_play_start_bindings;
/* Original296458 only: once-only env, real player pose/timer and optional
 * zero-frame black fade. It does not load actors or finish an active event. */
gh_game_event_status gh_game_event_play_start_00296458(const gh_event_play_start_bindings *,int32_t,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
