#ifndef GH_GAME_ACTOR_ATTACHED_SOUND_H
#define GH_GAME_ACTOR_ATTACHED_SOUND_H
#include "game_actor_motion.h"
#include "sound_scene.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One adapter, borrows SAME cSnd/controller/scene/voicepool/work/MT. No second
 * voicepool, sample registry or RNG. Keep thisowner untilvoicesEOS/abort. */
typedef struct {gh_se_voice_engine *voices;gh_sound_control *control;gh_sound_scene *scene;const gh_boot_work *work;gh_game_mt *random;} gh_actor_attached_sound_bindings;
typedef struct {gh_actor_attached_sound_bindings b;uint32_t frontier;int detail;uint8_t bound;} gh_actor_attached_sound;
gh_voice_status gh_actor_attached_sound_bind(gh_actor_attached_sound *,const gh_actor_attached_sound_bindings *);
/* Directrealprovider for motion.bindings.sound. Completes EEvoice construction
 * andownedPCM pendingqueue, actualoutput startsat ordinarySEupdate. */
gh_actor_motion_status gh_actor_attached_sound_motion(void *,const gh_motion_sound_request *,uint32_t *original_handle,uint32_t *frontier);
/* Hostshutdown/cancel only; call BEFORE anymodel/resource/pools release.
 * Stopsownednativevoices/cancelsreservations, notanoriginalroomcleanupack. */
gh_voice_status gh_actor_attached_sound_abort(gh_actor_attached_sound *);
#ifdef __cplusplus
}
#endif
#endif
