#ifndef GODHAND_CORE_SOUND_CONTROL_H
#define GODHAND_CORE_SOUND_CONTROL_H
#include "se_voice.h"
#include "../platform/disc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Decoded fields of original cSnd005FEE00, not a guest-memory layout. This is
 * the shared live controller; callers must not create a separate zero shadow. */
typedef struct gh_sound_control {uint32_t flags_ac,flags_b0;int32_t bgm_state_13c;uint8_t initialized;} gh_sound_control;
/* Restore exact actual-ELF PT_LOAD values (including BSS), then require actual
 * startup bank readiness. Verified current startup/MC path has no other writes
 * to these fields except the original SE flush pending-bit operation. Not a
 * general arbitrary midgame sound-state reconstruction. */
gh_voice_status gh_sound_control_from_startup(gh_sound_control *,gh_disc *,const gh_audio_registry *);
/* Real original leaf: SetBgmState0, B0&=FFBFFFFF, AC&=FFBFFFFF in that order. */
gh_voice_status gh_sound_control_prepare_scene_002ca8c8(gh_sound_control *);
/* Original cSnd_flushPendingSe2CA5D8: clear04000000 before real supported voice
 * update. Audited MC descriptors never set that bit. Other SE event types are
 * rejected by se_voice, not acknowledged as an empty flush. */
gh_voice_status gh_sound_control_flush_se_002ca5d8(gh_sound_control *,gh_se_voice_engine *);
#ifdef __cplusplus
}
#endif
#endif
