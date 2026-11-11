#ifndef GODHAND_CORE_MC_FILE_SAVE_H
#define GODHAND_CORE_MC_FILE_SAVE_H
#include "mc_file_load.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t mode, output_mode;
    uint8_t event_room_override;
    uint16_t event_room;
    int clock_result;
    uint8_t clock[8]; /* original sceCdReadClock status, second, minute, hour, pad, day, month, year */
    uint32_t history_mask;
    const uint8_t *history;
    size_t history_bytes; /* 20 original 0x400-byte slot records */
} gh_mc_save_metadata;

/* Original 6758 normal payload. Calls full checkpoint capture and updates
 * its canonical destination before serializing. Mode1 requires RoomJump. */
gh_mc_load_status gh_mc_save_serialize_00006758(gh_game_checkpoint *,
    const gh_checkpoint_bindings *,const gh_game_door *,const gh_mc_save_metadata *,
    uint8_t out[GH_MC_SAVE_RECORD_BYTES],uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
