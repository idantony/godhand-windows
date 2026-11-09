#ifndef GH_SOUND_BANK_H
#define GH_SOUND_BANK_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { GH_SOUND_OK,GH_SOUND_ARGUMENT,GH_SOUND_FORMAT,
 GH_SOUND_BOUNDS,GH_SOUND_UNSUPPORTED } gh_sound_status;
typedef struct {const uint8_t *data;size_t size;} gh_sound_span;
typedef struct {
 gh_sound_span image,sections[4]; /* SES +10,+18,+20,+28 offset/size pairs */
 const char *external_afs; /* bounded NUL string referenced by SES+30 */
} gh_sound_bank;
typedef struct {uint32_t rate,samples;uint16_t highpass;
 uint8_t encoding,block_bytes,bits,channels,version,flags;
 size_t audio_offset; } gh_adx_header;
/* Read-only views require caller-owned bytes to remain resident. Only original
 * unrelocated SES1.1 images supported; no original SPU2/IOP readiness implied. */
gh_sound_status gh_sound_bank_open(const void *,size_t,gh_sound_bank *);
gh_sound_status gh_sound_afs_entry(const void *,size_t,uint32_t,gh_sound_span *);
/* Parse CRI ADX structure, including unsupported codec/encryption metadata.
 * Never decodes encrypted frames or treats parse success as playable. */
gh_sound_status gh_sound_adx_header(gh_sound_span,gh_adx_header *);
gh_sound_status gh_sound_adx_decode_supported(const gh_adx_header *);
#ifdef __cplusplus
}
#endif
#endif
