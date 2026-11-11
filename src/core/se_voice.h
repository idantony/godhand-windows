#ifndef GODHAND_CORE_SE_VOICE_H
#define GODHAND_CORE_SE_VOICE_H
#include "../platform/audio_startup.h"
#include "../platform/audio_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_se_voice_engine gh_se_voice_engine;
/* Optional once-only native sound SDK table/RNG setup, before any live voices.
 * Reads user's libsd through SAME disc, no executable module loading. */
gh_voice_status gh_se_voice_bind_pitch_reference(gh_se_voice_engine *,gh_disc *);
gh_voice_status gh_se_voice_pitch_state(const gh_se_voice_engine *,uint16_t *state);
struct gh_actor_model;
struct gh_game_scene_tables;
struct gh_sound_control;
/* Binds the same boot key, disc and live mixer as the sequence SDK.
 * The actual R100 AFD sequence path remains strict until bound. */
gh_voice_status gh_se_voice_bind_external_adx(gh_se_voice_engine *,gh_disc *,
 const struct gh_sound_control *,const void *boot_elf,size_t boot_elf_bytes);
typedef struct {uint32_t system_flags_5e4,game_flags_5d8,gain64_bits;int32_t debug58c;uint16_t se_level_q8;uint8_t output_mode,reverb;
 uint32_t listener_position_c0[4],listener_matrix_d0[16];int32_t listener_mode110;
 uint32_t *sound_flags_ac,*sound_flags_b0;
 struct gh_game_scene_tables *scene_tables;} gh_se_attached_environment;
typedef gh_voice_status (*gh_se_attached_probe)(void *,const float position[4],int check_seh,gh_se_attached_environment *,uint32_t *frontier);
typedef gh_voice_status (*gh_se_attached_random)(void *,uint32_t *word,uint32_t *frontier);
typedef enum gh_se_dsp_policy {GH_SE_DSP_NATIVE_LINEAR_REFERENCE=1} gh_se_dsp_policy;
typedef struct gh_se_mix {uint16_t se_level_q8;uint8_t output_mode;uint32_t system_flags;gh_se_dsp_policy dsp_policy;} gh_se_mix;
/* Scalar mixer portion of actual boot settings2C9E78→375630: stereo and
 * SFX90/127 of master127. Does not claim the original global frame/RPC calls. */
gh_voice_status gh_se_mix_boot_defaults_002c9e78(gh_se_mix *,uint32_t system_flags,gh_se_dsp_policy);
typedef struct gh_se_voice_info {uint32_t original_handle,native_voice,sample_slot,frames,rate;uint16_t pitch_q12,adsr1,adsr2,level_q14;int16_t program,bank;uint8_t pending,channel;int16_t stop_current,stop_step;uint16_t stop_count;uint8_t stopping,retiring;
 const struct gh_actor_model *attached_model;int32_t attached_part;float source_position[4];uint8_t attached,position_mode;
 uint16_t distance,azimuth,pan,left_q14,right_q14;} gh_se_voice_info;
/* Narrow real origin MC event path: core bank0 programs2D/15E/15F/160/161 and R020 bank2 programs0/10/11/12, original
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
/* Actual2CBA48/2CDD18 actor binding into SAME64EEpool. request_key identifies
 * ONLY an unfinished invocation; completion releaseskey, sameaddressreuse is
 * a NEWoriginalcall. owner/probecontext must live untilabort orallvoicesEOS.
 * Current finite mode0/bootmix/one-shot DSP profile; unknownspatial/flags retain
 * their exactfrontier. MT runs onlyAFTER firstfreeslot andonlyonce percall. */
gh_voice_status gh_se_voice_request_attached(gh_se_voice_engine *,const void *owner,const void *request_key,struct gh_actor_model *,int32_t part_index,int16_t bank,int16_t program,uint32_t flags,int32_t delay,gh_se_attached_probe,void *probe_context,gh_se_attached_random,void *random_context,uint32_t *original_handle);
/* Actual2CE488 clearsactor/part refs, keepsvoice/PCM; no modeldtor claim. */
gh_voice_status gh_se_voice_detach_actor_002ce488(gh_se_voice_engine *,struct gh_actor_model *);
/* Explicit native shutdown abort (NOT originalDetach/Stop). Dropsonlyowner's
 * reservations/voices, balancesborrow, stopsrealdevice sourceifissued. */
gh_voice_status gh_se_voice_abort_attached_owner(gh_se_voice_engine *,const void *owner);
unsigned gh_se_voice_attached_reservations(const gh_se_voice_engine *);
typedef struct {
 uint32_t frontier,flags;
 int32_t part_index,delay;
 int16_t bank,program;
 gh_voice_status status;
} gh_se_attached_failure;
/* Most recent attached request with a known original boundary. */
void gh_se_voice_last_attached_failure(const gh_se_voice_engine *,gh_se_attached_failure *);
/* Original active-list (newest first), descriptor-selected channel groups0..4 and15..16, priority/oldest
 * replacement semantics for audited MC/logo records. Starts real
 * XAudio2 voices. Call from original sound update boundary once per frame. */
gh_voice_status gh_se_voice_update(gh_se_voice_engine *);
gh_voice_status gh_se_voice_query(const gh_se_voice_engine *,uint32_t handle,gh_se_voice_info *);
/* Borrowed pending native PCM; invalid after update/destroy. Read-only QA/view. */
gh_voice_status gh_se_voice_pcm_view(const gh_se_voice_engine *,uint32_t handle,const int16_t **,size_t *frames);
/* Actual375050 queues a stop against the same native-bound SDE owner.
 * Shared48-command bound; accepted=0 is original queue-full failure. Commands
 * consume at ordinary update, never at request time. Native busy detection
 * uses real SamplesPlayed; integer ramp matches37D8B8/378020/378070/37CC78.
 * Native source gain replaces SPU volume/keyoff timing, not hardware DSP. */
gh_voice_status gh_se_voice_request_stop_00375050(gh_se_voice_engine *,uint32_t original_handle,int16_t ticks,int *accepted);
/* Original2CBC58 matches EE node+08 BANK, not node+0A program. Active
 * voices queue real375050 stops; unissued pending nodes are removed now.
 * Original void ignores queue-full; accepted48-command bound is preserved. */
gh_voice_status gh_se_voice_stop_bank_002cbc58(gh_se_voice_engine *,int16_t bank,int16_t ticks);
/* Original ascending bank0..51 stops10 plus pending EE-node removal. */
unsigned gh_se_voice_bank_nodes(const gh_se_voice_engine *,int16_t bank);
gh_voice_status gh_se_voice_scene_exit_begin(gh_se_voice_engine *);
int gh_se_voice_uses_registry(const gh_se_voice_engine *,const gh_audio_registry *);
unsigned gh_se_voice_active_nodes(const gh_se_voice_engine *);
unsigned gh_se_voice_external_adx_nodes(const gh_se_voice_engine *);
uint32_t gh_se_voice_frontier(const gh_se_voice_engine *);
/* Frontier addresses below100000 refer to user snddrv.irx; larger addresses
 * refer to boot ELF. No guest address is invoked as a host function pointer. */
#ifdef __cplusplus
}
#endif
#endif
