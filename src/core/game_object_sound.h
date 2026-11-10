#ifndef GH_GAME_OBJECT_SOUND_H
#define GH_GAME_OBJECT_SOUND_H
#include "sound_control.h"
#include "game_objects.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_sound_control *control;gh_se_voice_engine *voices;} gh_object_sound_binding;
/* Exact original recent push; future nonzero variant implementations must
 * execute this at2CE134, not initialize a second ring or silently skip it. */
gh_voice_status gh_sound_recent_push_002cbdc0(gh_sound_control *,int16_t bank,int16_t program,uint32_t key);
/* Current native descriptor profile has variant_count(+21)==0 for every
 * supported call, so2CE144/14C skip the ONLY ring writer2CE134. This release
 * handles originalslot-1's proven alias to recent[10], only when actualvoice
 * head is empty. It is not375050 driver stop, bankfree or an invalid-slotnoop. */
gh_object_status gh_sound_object_slot_release_002cb340(void *,int32_t slot,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
