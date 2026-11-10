#ifndef GH_ASSETS_SE_PITCH_H
#define GH_ASSETS_SE_PITCH_H
#include "../platform/disc.h"
#include "../platform/audio_events.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One SDE sound RNG + runtime-loaded original libsd note tables. Not EE MT. */
typedef struct {uint16_t semitones[12],fine[128],random_state;uint8_t loaded;} gh_se_pitch_reference;
gh_voice_status gh_se_pitch_load_reference(gh_se_pitch_reference *,gh_disc *);
gh_voice_status gh_se_pitch_choose_0037d760(gh_se_pitch_reference *,int16_t low,int16_t high,int16_t *chosen);
gh_voice_status gh_se_pitch_q12_000038e4(const gh_se_pitch_reference *,int16_t offset,uint32_t rate,uint16_t *pitch);
#ifdef __cplusplus
}
#endif
#endif
