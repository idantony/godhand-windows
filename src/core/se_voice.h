#ifndef GODHAND_CORE_SE_VOICE_H
#define GODHAND_CORE_SE_VOICE_H
#include "../platform/audio_startup.h"
#include "../platform/audio_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_se_voice_engine gh_se_voice_engine;
typedef enum gh_se_dsp_policy {GH_SE_DSP_NATIVE_LINEAR_REFERENCE=1} gh_se_dsp_policy;
typedef struct gh_se_mix {uint16_t se_level_q8;uint8_t output_mode;uint32_t system_flags;gh_se_dsp_policy dsp_policy;} gh_se_mix;
/* Scalar mixer portion of actual boot settings2C9E78→375630: stereo and
 * SFX90/127 of master127. Does not claim the original global frame/RPC calls. */
gh_voice_status gh_se_mix_boot_defaults_002c9e78(gh_se_mix *,uint32_t system_flags,gh_se_dsp_policy);
typedef struct gh_se_voice_info {uint32_t original_handle,native_voice,sample_slot,frames,rate;uint16_t pitch_q12,adsr1,adsr2,level_q14;int16_t program;uint8_t pending,channel;} gh_se_voice_info;
/* Narrow real origin MC event path: core bank0 programs2D/15E/160/161, original
 * default args(-1,-1,0,0), stereo/mono and explicit original SFX master level.
 * Caller supplies real registry/device; no emulated device or empty command
 * acknowledgment. Hardware DSP becomes a documented native linear resampler
 * plus fixed-envelope reference model, not SPU2 bit-identical output. */
gh_voice_status gh_se_voice_create(const gh_audio_registry *,gh_audio_output *,
 const gh_se_mix *,gh_se_voice_engine **);
/* Explicit borrowed original cSnd+20 SEH context. NULL/0 means verified absent.
 * Required for logo2D; nonempty tables remain unsupported at2CAE10. Rebind
 * when canonical scene changes, before issuing its origin event. */
gh_voice_status gh_se_voice_bind_origin_seh(gh_se_voice_engine *,const void *,size_t);
void gh_se_voice_destroy(gh_se_voice_engine *);
/* Allocate first free of original64 EE voices, copy/resolve real descriptor,
 * decode and own PCM, prepend to active list. Original handle0 on pool full.
 * Unsupported data returns exact first_unported; no command is queued. */
gh_voice_status gh_se_voice_request_origin(gh_se_voice_engine *,int16_t bank,int16_t program,
 int16_t arg3,int16_t arg4,uint32_t flags,int32_t delay,uint32_t *original_handle);
/* Original active-list (newest first), descriptor-selected channel groups0..4 and15..16, priority/oldest
 * replacement semantics for audited MC/logo records. Starts real
 * XAudio2 voices. Call from original sound update boundary once per frame. */
gh_voice_status gh_se_voice_update(gh_se_voice_engine *);
gh_voice_status gh_se_voice_query(const gh_se_voice_engine *,uint32_t handle,gh_se_voice_info *);
/* Borrowed pending native PCM; invalid after update/destroy. Read-only QA/view. */
gh_voice_status gh_se_voice_pcm_view(const gh_se_voice_engine *,uint32_t handle,const int16_t **,size_t *frames);
uint32_t gh_se_voice_frontier(const gh_se_voice_engine *);
/* Frontier addresses below100000 refer to user snddrv.irx; larger addresses
 * refer to boot ELF. No guest address is invoked as a host function pointer. */
#ifdef __cplusplus
}
#endif
#endif
