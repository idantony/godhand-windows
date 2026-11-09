#ifndef GH_AUDIO_STARTUP_H
#define GH_AUDIO_STARTUP_H
#include "../assets/sound_bank.h"
#include "disc.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_audio_registry gh_audio_registry;
typedef enum { GH_AUDIO_OK,GH_AUDIO_ARGUMENT,GH_AUDIO_FORMAT,GH_AUDIO_BOUNDS,
 GH_AUDIO_MEMORY,GH_AUDIO_NOT_FOUND,GH_AUDIO_DISABLED,GH_AUDIO_UNSUPPORTED } gh_audio_status;
typedef struct {uint64_t generation;int16_t bank;
 const gh_audio_registry *owner;} gh_audio_token;
/* Native replacement for SDE bank commits3/6 and transfer-owned lifetime.
 * This is a usable registry, not a SIF RPC device/return-code emulation.
 * No voices, SPU2 output or ADX decoding implemented. One owner thread only. */
gh_audio_status gh_audio_registry_create(gh_audio_registry **);
void gh_audio_registry_destroy(gh_audio_registry *);
/* Atomic commit. Deep-copies all SES data; verifies all table ranges before
 * publishing. Absent AFD (offset/size0) is valid, with no external directory.
 * Failed replacement preserves prior generation and bank. */
gh_audio_status gh_audio_commit_se(gh_audio_registry *,int16_t bank,
 const void *ses,size_t bytes,gh_audio_token *);
int gh_audio_token_complete(const gh_audio_registry *,gh_audio_token);
gh_audio_status gh_audio_release_bank(gh_audio_registry *,int16_t bank);
/* SdeIssTblGet374CB8 / event-table375AB0: selected 48-byte descriptor copied,
 * disabled bit8000 at+0e rejected; first shorts replaced with bank/index.
 * event_table==0 selects original count/table at header+0/+4;
 * event_table==1 selects header+14/+18. Negative host indices rejected. */
gh_audio_status gh_audio_get_program(const gh_audio_registry *,int16_t bank,
 int32_t index,int event_table,uint8_t descriptor[48]);
/* Exact cmd3/cmd6 mapping fields, all views point into owned committed memory.
 * programs=SES+10; sys_table=SES+20; sample_bytes=SES+28; afd=SES+18.
 * Raw table fields beyond proven offsets retain their unknown meanings. */
gh_audio_status gh_audio_bank_views(const gh_audio_registry *,int16_t bank,
 gh_sound_span *programs,gh_sound_span *sys_table,gh_sound_span *sample_bytes,
 gh_sound_span *afd);
/* Resolve native sample descriptor by table slot. The first u32 is a byte
 * offset into owned sample bytes. Returns a bounded tail, NOT an inferred
 * sample length/duration. No guessed conversion from event fields to slot. */
gh_audio_status gh_audio_sample_at(const gh_audio_registry *,int16_t bank,
 uint32_t slot,gh_sound_span *descriptor,gh_sound_span *sample_tail);
/* Synchronous local-disk timing adaptation of2CA798. Real reads/owned bank
 * commit and external AFS directories finish before return. No dummy frame
 * sleeps; no SPU2/voice/output capability is claimed. Returns on first error,
 * retaining partial real banks. Safe to retry: each bank replaces atomically. */
gh_audio_status gh_audio_startup_load(gh_audio_registry *,gh_disc *);
int gh_audio_startup_ready(const gh_audio_registry *,uint32_t *core_flags,
 uint32_t *pl00_flags,uint32_t *bgm_flags);
/* BGM0 AFD is owned; directory offsets/sizes are original bytes in bgm00.afs.
 * Returns NOT_FOUND until corresponding actual directory is committed.
 * bank=-1 selects BGM0, otherwise actual SE bank id. */
gh_audio_status gh_audio_afs_member(const gh_audio_registry *,int16_t bank,
 uint32_t index,uint64_t *offset,uint64_t *size);
/* Actual373298 selects AFD row*32;373D90 passes row+2 member number to
 *32BB48. Resolve that number against owned external AFS directory. */
gh_audio_status gh_audio_external_sample(const gh_audio_registry *,int16_t bank,
 uint32_t afd_index,gh_sound_span *afd_record,uint64_t *offset,uint64_t *size);
gh_audio_status gh_audio_bgm0_table(const gh_audio_registry *,gh_sound_span *);
#ifdef __cplusplus
}
#endif
#endif
