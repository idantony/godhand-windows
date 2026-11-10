#ifndef GODHAND_CORE_SOUND_CONTROL_H
#define GODHAND_CORE_SOUND_CONTROL_H
#include "se_voice.h"
#include "../platform/disc.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Decoded fields of original cSnd005FEE00, not a guest-memory layout. This is
 * the shared live controller; callers must not create a separate zero shadow. */
/* Native SPU-allocation replacement view, not a PS2 heap byte-layout.
 * Registry owns the actual sample bytes. Closed is a native release milestone,
 * never a guessed guest self380 flag or parent pointer. */
typedef struct {const gh_audio_registry *registry;uint32_t original_global;
 uint8_t identity_verified,closed;} gh_sound_heap_registry_owner;
typedef struct {int16_t bank,program;uint32_t key;} gh_sound_recent;
typedef struct gh_sound_control {uint32_t flags_ac,flags_b0;int32_t bgm_state_13c;uint8_t initialized;
 /* Actual2C9E78 bootgainfloatbits at+0..14, and2C9F58 +50 eventmask.
  * One canonical source for BGM/SE consumers, not separate bootshadows. */
 uint32_t event_mask50,gain_bits[6];uint8_t output_mode;
 /* RemainingcanonicalcSnd scalar/listenerfields usedbyactual2CA518. */
 uint32_t previous_mask54,event_mask58,base_mask5c;
 uint32_t listener_position_c0[4],listener_matrix_d0[16];
 int32_t listener_mode110;void *ts_head2c;
 uint32_t unload_a4;gh_sound_heap_registry_owner room_heaps[2];
 /* ONE actual160..1DF ring and1E0 index. SupportedSE variants currently0. */
 gh_sound_recent recent[16];int32_t recent_pos;uint8_t recent_initialized;} gh_sound_control;
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
