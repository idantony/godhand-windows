#ifndef GODHAND_PLATFORM_AUDIO_EVENTS_H
#define GODHAND_PLATFORM_AUDIO_EVENTS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum gh_voice_status {GH_VOICE_OK,GH_VOICE_ARGUMENT,GH_VOICE_BOUNDS,GH_VOICE_UNSUPPORTED,GH_VOICE_MEMORY,GH_VOICE_PLATFORM,GH_VOICE_CAPACITY} gh_voice_status;
typedef struct gh_adpcm_pcm {int16_t *samples;size_t frames,source_bytes;} gh_adpcm_pcm;
/* One bounded mono PS2 ADPCM one-shot, zero predictor history. Stops after
 * END block. Loop/repeat/unknown coefficients/flags return UNSUPPORTED, not a
 * shortened successful voice. Caller owns returned memory; release below. */
gh_voice_status gh_audio_decode_ps_adpcm(const void *,size_t,gh_adpcm_pcm *);
void gh_audio_pcm_release(gh_adpcm_pcm *);
typedef struct gh_audio_output gh_audio_output;
/* Real XAudio2.9 device, one owner thread. No device => PLATFORM, never a dummy
 * output. Platform PCM transport only; not the game's SeCall/ADSR interpreter. */
gh_voice_status gh_audio_output_create(gh_audio_output **,int32_t *native_error);
void gh_audio_output_destroy(gh_audio_output *);
/* Native user/output gain; zero still processes real source buffers. */
gh_voice_status gh_audio_output_set_gain(gh_audio_output *,float gain);
/* Copies PCM until actual XAudio2 buffer completion; L/R gains0..1 explicit.
 * Successful submit/start is real voice consumption, not proof of audibility. */
gh_voice_status gh_audio_output_play(gh_audio_output *,const gh_adpcm_pcm *,
 uint32_t rate,float left,float right,uint32_t *voice);
/* Original SPU2 pitch register units (4096 = 48000Hz). Host resampling replaces
 * hardware interpolation; preserves the original quantized playback rate. */
gh_voice_status gh_audio_output_play_q12(gh_audio_output *,const gh_adpcm_pcm *,
 uint16_t pitch_q12,float left,float right,uint32_t *voice);
gh_voice_status gh_audio_output_active(gh_audio_output *,uint32_t voice,int *active,uint64_t *played);
/* Absolute native per-source output matrix; does not touch master mute.
 * Original integer gain computation stays in core, transport accepts0..1. */
gh_voice_status gh_audio_output_voice_gain(gh_audio_output *,uint32_t voice,float left,float right);
gh_voice_status gh_audio_output_voice_gain_query(gh_audio_output *,uint32_t voice,float *left,float *right);
gh_voice_status gh_audio_output_stop(gh_audio_output *,uint32_t voice);
void gh_audio_output_reap(gh_audio_output *);
#ifdef __cplusplus
}
#endif
#endif
