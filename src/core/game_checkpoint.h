#ifndef GH_GAME_CHECKPOINT_H
#define GH_GAME_CHECKPOINT_H
#include "game_prepare.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_CHECKPOINT_BYTES 0xb0f0u
/* Original755880 restarttail, scalarbinary32bits, no hostfloatarithmetic. */
typedef struct {uint32_t position[3],rotation[3],heat,heat_max;} gh_checkpoint_tail32;
typedef struct {gh_checkpoint_tail32 tail;uint32_t frontier;uint8_t captured,last_skipped;} gh_game_checkpoint;
typedef struct {gh_boot_work *work;gh_core_save_view save;gh_game_prepare *prepare;
 gh_game_init_state *init;const gh_title_player_view *player;
 uint8_t *destination;size_t destination_size;} gh_checkpoint_bindings;
typedef enum {GH_CHECKPOINT_OK,GH_CHECKPOINT_SKIPPED,GH_CHECKPOINT_ARGUMENT,
 GH_CHECKPOINT_BOUNDS,GH_CHECKPOINT_UNSUPPORTED} gh_checkpoint_status;
/* Original direct full0 checkpoint with NULL position and rotation overrides.
 * The scene-entry suppression flags belong to 2BF8F8, not to this call. */
gh_checkpoint_status gh_game_checkpoint_002c05b0(gh_game_checkpoint *,const gh_checkpoint_bindings *);
/* Actual2BF8F8 gate->2C05B0(full0,posNULL,rotNULL), including1F9B88snapshot
 * subset/31C350 and14A0+9C30+20 copy into canonicalstage.work_area.
 * Unknownplayer54A returns precise1F9BEC AFTER originalCoreSaveprefixwrites;
 * destination remainsuntouched. Explicitmemoryprofilemustsupplythatinput. */
gh_checkpoint_status gh_game_checkpoint_002bf8f8(gh_game_checkpoint *,const gh_checkpoint_bindings *);
#ifdef __cplusplus
}
#endif
#endif
